#pragma once

#include <map>
#include <string>
#include <memory>

#include <rapidjson/document.h>

#include "ShaderProgram.h"

class ShaderPool {
public:
	static void AddShader(
		const std::string& shaderName,
		const std::string& baseFile,
		ShaderProgram::ShaderProgramType type = ShaderProgram::RenderShader,
		const std::vector<std::string>& defines = {},
		const std::map<std::string, std::string> & snippets = {});

	/**
	 * Copy a base shader and add an extra define. If the define is already
	 * activated in the original shader, this creates an alias pointing to the
	 * previous shader, not a deep copy. This has no effect if there is no
	 * shader called baseShaderName.
	 */
	static void AddShaderVariant(
		const std::string & shaderName,
		const std::string & baseShaderName,
		const std::string & define);

	static void AddShaderVariant(
		const std::string& shaderName,
		const std::string& baseShaderName,
		const std::vector<std::string>& defines,
		const std::map<std::string, std::string>& snippets = {});

	/**
	 * Get a shader from the pool by its name.
	 */
	static std::shared_ptr<ShaderProgram> GetShader(const std::string & shaderName);

	static void ReloadShaders();

	/**
	 * Load shaders from JSON file
	 */
	static bool Deserialize(const rapidjson::Value & json);

	/**
	 * Remove all shaders from pool (invalidates all weak pointers to it
	 */
	static void Clear();

private:
	ShaderPool();
	ShaderPool& operator=(const ShaderPool&) = default;
	ShaderPool(const ShaderPool&) = default;

	void addShader(
		const std::string& shaderName,
		const std::string& baseFile,
		ShaderProgram::ShaderProgramType type,
		const std::vector<std::string>& defines = {},
		const std::map<std::string, std::string>& snippets = {});

	void addShaderVariant(
		const std::string& shaderName,
		const std::string& baseShaderName,
		const std::vector<std::string>& defines,
		const std::map<std::string, std::string>& snippets = {});

	std::shared_ptr<ShaderProgram> getShader(const std::string & shaderName);

	void reloadShaders();
	bool deserialize(const rapidjson::Value & json);
	void clear();

private:
	struct ShaderInfo {
		std::string baseFile;
		ShaderProgram::ShaderProgramType type;
		std::vector<std::string> defines;
	};

private:
	static ShaderPool s_instance;
	std::map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
	std::map<std::string, ShaderInfo> m_defaultShaders;
};
