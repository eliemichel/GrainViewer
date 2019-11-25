#pragma once

#include <map>
#include <string>
#include <memory>

#include <rapidjson/document.h>

#include "ShaderProgram.h"

class ShaderPool {
public:
	static void AddShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines);

	/**
	 * Copy a base shader and add an extra define. If the define is already
	 * activated in the original shader, this creates an alias pointing to the
	 * previous shader, not a deep copy. This has no effect if there is no
	 * shader called baseShaderName.
	 */
	static void AddShaderVariant(const std::string & shaderName, const std::string & baseShaderName, const std::string & define);

	/**
	 * Get a shader from the pool by its name.
	 */
	static std::shared_ptr<ShaderProgram> GetShader(const std::string & shaderName);

	static void ReloadShaders();

	static bool Deserialize(const rapidjson::Value & json);

private:
	ShaderPool() {}
	ShaderPool& operator=(const ShaderPool&) {}
	ShaderPool(const ShaderPool&) {}

	void addShader(const std::string & shaderName, const std::string & baseFile, const std::vector<std::string> & defines);
	void addShaderVariant(const std::string & shaderName, const std::string & baseShaderName, const std::string & define);
	std::shared_ptr<ShaderProgram> getShader(const std::string & shaderName) const;
	void reloadShaders();
	bool deserialize(const rapidjson::Value & json);

private:
	static ShaderPool s_instance;
	std::map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
};
