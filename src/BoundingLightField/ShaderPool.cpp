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

std::shared_ptr<ShaderProgram> ShaderPool::GetShader(const std::string & shaderName)
{
	return s_instance.getShader(shaderName);
}

void ShaderPool::ReloadShaders()
{
	s_instance.reloadShaders();
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
