#include <sstream>

#include "Logger.h"
#include "ShaderPool.h"

ShaderPool ShaderPool::s_instance = ShaderPool();

///////////////////////////////////////////////////////////////////////////////
// Public static methods
///////////////////////////////////////////////////////////////////////////////

void ShaderPool::AddShader(const std::string & shaderName, const std::string & baseFile, ShaderProgram::ShaderProgramType type, const std::vector<std::string> & defines)
{
	s_instance.addShader(shaderName, baseFile, type, defines);
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

ShaderPool::ShaderPool()
{
	m_defaultShaders.insert({
		"PrefixSum",
		{ "prefix-sum", ShaderProgram::ComputeShader, {} }
	});
}

void ShaderPool::addShader(const std::string & shaderName, const std::string & baseFile, ShaderProgram::ShaderProgramType type, const std::vector<std::string> & defines)
{
	if (m_shaders.count(shaderName) > 0) {
		WARN_LOG << "Duplicate shader name: " << shaderName << " (ignoring duplicates)";
		return;
	}
	m_shaders[shaderName] = std::make_shared<ShaderProgram>(baseFile);
	m_shaders[shaderName]->setType(type);
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

std::shared_ptr<ShaderProgram> ShaderPool::getShader(const std::string & shaderName)
{
	if (m_shaders.count(shaderName) > 0) {
		return m_shaders.at(shaderName);
	} else if (m_defaultShaders.count(shaderName) > 0) {
		auto& info = m_defaultShaders[shaderName];
		addShader(shaderName, info.baseFile, info.type, info.defines);
		return m_shaders.at(shaderName);
	} else {
		return nullptr;
	}
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
		ShaderProgram::ShaderProgramType type = ShaderProgram::RenderShader;
		std::ostringstream definesDebug;
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
					if (i > 0) definesDebug << ", ";
					definesDebug << def.GetString();
				}
			}
			if (it->value.HasMember("type") && it->value["type"].IsString()) {
				std::string strtype = it->value["type"].GetString();
				if (strtype == "compute") {
					type = ShaderProgram::ComputeShader;
				} else if (strtype == "render") {
					type = ShaderProgram::RenderShader;
				}
				else {
					ERR_LOG << "Shader type not recognized: " << strtype << " (in shader '" << name << "')";
					continue;
				}
			}
		}
		else {
			ERR_LOG << "Shader entry must be either a string ofr an object. Ignoring shader '" << name << "'";
		}
		addShader(name, baseFile, type, defines);
		LOG << " - shader '" << name << "' loaded with defines [" << definesDebug.str() << "]";
	}
	return true;
}
