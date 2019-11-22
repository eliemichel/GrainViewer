#pragma once

#include "Shader.h"

/**
 * Utility class providing an OO API to OpenGL shader program
 */
class ShaderProgram {
public:
	inline static void setRoot(const std::string& root) { s_root = root; }
	inline static const std::string& root() { return s_root; }

	/**
	 * Resolve full file path for a given shader type. This is used internally
	 * by load() but might also be used externally to monitor the provided
	 * files and call load whenever they change.
	 */
	static std::string shaderFullPath(const std::string& shaderName, GLenum type);

	/**
	 * List all relevant shader files returned by shaderFullPath, to be used
	 * externally to monitor the provided files and call load whenever they change.
	 */
	static std::vector<std::string> allShaderFullPaths(const std::string& shaderName);

private:
	static std::string s_root;

public:
	ShaderProgram(const std::string& shaderName = "");
	ShaderProgram(ShaderProgram&&) = default;
	ShaderProgram& operator=(ShaderProgram&&) = default;

	/**
	 * NB: Changing shader name does not reload it. You may want to call load() then.
	 */
	inline void setShaderName(const std::string& shaderName) { m_shaderName = shaderName; }
	inline const std::string& shaderName() const { return m_shaderName; }

	inline void define(const std::string& def) { m_defines.insert(def); }
	inline void undefine(const std::string& def) { m_defines.erase(def); }

	inline void setSnippet(const std::string& key, const std::string& value) { m_snippets[key] = value; }

	/**
	 * Load and check shaders
	 */
	void load();

	/**
	 * Check that the shader program has been successfully compiled
	 * @param name Name displayed in error message
	 * @param success status
	 */
	bool check(const std::string& name = "shader program") const;

	inline void use() const { if (isValid()) glUseProgram(m_programId); }
	inline bool isValid() const { return m_isValid; }

	// Some overloading: (add whatever you need)
	void setUniform(const std::string& name, GLint value) const;
	void setUniform(const std::string& name, GLuint value) const;
	void setUniform(const std::string& name, GLfloat value) const;
	void setUniform(const std::string& name, const glm::vec2& value) const;
	void setUniform(const std::string& name, const glm::vec3& value) const;
	void setUniform(const std::string& name, const glm::mat3& value) const;
	void setUniform(const std::string& name, const glm::mat4& value) const;

	bool bindUniformBlock(const std::string& uniformBlockName, GLuint buffer, GLuint uniformBlockBinding = 1);

	std::vector<std::string> getUniformList() const;

private:
	std::string m_shaderName;
	std::set<std::string> m_defines;
	std::map<std::string, std::string> m_snippets;
	GLuint m_programId;
	bool m_isValid;

private:
	inline GLint uniformLocation(const std::string& name) const { return m_isValid ? glGetUniformLocation(m_programId, name.c_str()) : -1; }
	inline GLuint uniformBlockIndex(const std::string& name) const { return m_isValid ? glGetUniformBlockIndex(m_programId, name.c_str()) : GL_INVALID_INDEX; }
	inline GLuint storageBlockIndex(const std::string& name) const { return m_isValid ? glGetProgramResourceIndex(m_programId, GL_SHADER_STORAGE_BLOCK, name.c_str()) : GL_INVALID_INDEX; }
};

