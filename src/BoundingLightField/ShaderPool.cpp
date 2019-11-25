#include "Logger.h"
#include "ShaderPool.h"

ShaderPool ShaderPool::s_instance = ShaderPool();

///////////////////////////////////////////////////////////////////////////////
// Public static methods
///////////////////////////////////////////////////////////////////////////////

void ShaderPool::AddShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines)
{
	s_instance.addShader(shaderName, baseFile, defines);
}

void ShaderPool::AddShaderVariant(const std::string & shaderName, const std::string & baseShaderName, const std::string & define)
{
	s_instance.addShaderVariant(shaderName, baseShaderName, define);
}

std::shared_ptr<ShaderProgram> ShaderPool::GetShader(const std::string & shaderName)
{
	return s_instance.getShader(shaderName);
}

void ShaderPool::ReloadShaders()
{
	s_instance.reloadShaders();
}

bool ShaderPool::Deserialize(const rapidjson::Value & json)
{
	return s_instance.deserialize(json);
}

///////////////////////////////////////////////////////////////////////////////
// Private singleton methods
///////////////////////////////////////////////////////////////////////////////

void ShaderPool::addShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines)
{
	if (m_shaders.count(shaderName) > 0) {
		WARN_LOG << "Duplicate shader name: " << shaderName << " (ignoring duplicates)";
		return;
	}
	m_shaders[shaderName] = std::make_shared<ShaderProgram>(baseFile);
	for (const std::string& def : defines) {
		m_shaders[shaderName]->define(def);
	}
	m_shaders[shaderName]->load();
}

void ShaderPool::addShaderVariant(const std::string & shaderName, const std::string & baseShaderName, const std::string & define)
{
	auto baseShader = getShader(baseShaderName);
	if (!baseShader) {
		WARN_LOG << "Cannot add variant to unexistant shader: " << baseShaderName;
		return;
	}

	if (baseShader->getDefines().count(define) > 0) {
		m_shaders[shaderName] = baseShader;
	}
	else {
		m_shaders[shaderName] = std::make_shared<ShaderProgram>(baseShader->shaderName());
		m_shaders[shaderName]->copy(*baseShader);
		m_shaders[shaderName]->define(define);
		m_shaders[shaderName]->load();
	}
}

std::shared_ptr<ShaderProgram> ShaderPool::getShader(const std::string & shaderName) const
{
	return m_shaders.count(shaderName) > 0 ? m_shaders.at(shaderName) : nullptr;
}

void ShaderPool::reloadShaders()
{
	LOG << "Reloading shaders...";
	for (auto const& e : m_shaders)
	{
		e.second->load();
	}
}

bool ShaderPool::deserialize(const rapidjson::Value & json)
{
	LOG << "Loading shaders...";
	if (!json.IsObject()) { ERR_LOG << "shaders field must be an object."; return false; }

	for (auto it = json.MemberBegin(), end = json.MemberEnd(); it != end; ++it) {
		std::string name = it->name.GetString();
		std::vector<std::string> defines;
		std::string baseFile;
		if (it->value.IsString()) {
			baseFile = it->value.GetString();
		}
		else if (it->value.IsObject()) {
			if (it->value.HasMember("baseFile") && it->value["baseFile"].IsString()) {
				baseFile = it->value["baseFile"].GetString();
			}
			if (it->value.HasMember("defines") && it->value["defines"].IsArray()) {
				for (rapidjson::SizeType i = 0; i < it->value["defines"].Size(); i++) {
					auto& def = it->value["defines"][i];
					if (!def.IsString()) {
						ERR_LOG << "Shader defines must be strings (in shader '" << name << "')";
						continue;
					}
					defines.push_back(def.GetString());
				}
			}
		}
		else {
			ERR_LOG << "Shader entry must be either a string ofr an object. Ignoring shader '" << name << "'";
		}
		addShader(name, baseFile, defines);
		LOG << " - shader '" << name << "' loaded";
	}
	return true;
}
