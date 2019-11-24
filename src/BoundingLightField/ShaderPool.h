#pragma once

#include <map>
#include <string>
#include <memory>

#include "ShaderProgram.h"

class ShaderPool {
public:
	static void AddShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines);
	static std::shared_ptr<ShaderProgram> GetShader(const std::string & shaderName);
	static void ReloadShaders();

private:
	ShaderPool() {}
	ShaderPool& operator=(const ShaderPool&) {}
	ShaderPool(const ShaderPool&) {}

	void addShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines);
	std::shared_ptr<ShaderProgram> getShader(const std::string & shaderName) const;
	void reloadShaders();

private:
	static ShaderPool s_instance;
	std::map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
};
