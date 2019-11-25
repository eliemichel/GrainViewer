#include <glm/gtc/type_ptr.hpp>

#include "utils/fileutils.h"
#include "Logger.h"
#include "ResourceManager.h"
#include "ShaderProgram.h"

ShaderProgram::ShaderProgram(const std::string& shaderName)
	: m_shaderName(shaderName)
	, m_isValid(false)
{}

void ShaderProgram::load() {
	m_programId = glCreateProgram();

	std::vector<std::string> defines(m_defines.begin(), m_defines.end());

	Shader vertexShader(GL_VERTEX_SHADER);
	vertexShader.load(ResourceManager::shaderFullPath(m_shaderName, GL_VERTEX_SHADER), defines, m_snippets);
	vertexShader.compile();
	vertexShader.check("vertex shader");
	glAttachShader(m_programId, vertexShader.shaderId());

	Shader geometryShader(GL_GEOMETRY_SHADER);
	std::string geometryShaderPath = ResourceManager::shaderFullPath(m_shaderName, GL_GEOMETRY_SHADER);
	if (isFile(geometryShaderPath)) {
		geometryShader.load(geometryShaderPath, defines, m_snippets);
		geometryShader.compile();
		geometryShader.check("geometry shader");
		glAttachShader(m_programId, geometryShader.shaderId());
	}

	Shader fragmentShader(GL_FRAGMENT_SHADER);
	fragmentShader.load(ResourceManager::shaderFullPath(m_shaderName, GL_FRAGMENT_SHADER), defines, m_snippets);
	fragmentShader.compile();
	fragmentShader.check("fragment shader");
	glAttachShader(m_programId, fragmentShader.shaderId());

	glLinkProgram(m_programId);
	m_isValid = check();
}

bool ShaderProgram::check(const std::string& name) const {
	int ok;

	glGetProgramiv(m_programId, GL_LINK_STATUS, &ok);
	if (!ok) {
		int len;
		glGetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &len);
		char* log = new char[len];
		glGetProgramInfoLog(m_programId, len, &len, log);
		ERR_LOG
			<< "ERROR: Unable to link " << name << " (" << m_shaderName << "). OpenGL returned:" << std::endl
			<< log << std::endl;
		delete[] log;
	}

	return ok;
}


void ShaderProgram::setUniform(const std::string& name, GLint value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glUniform1i(loc, value);
	}
	else {
		// Problem with this warning: it is repeated at every call to update.
		// There should be either a caching mechanism to avoid repeating it
		// for a given uniform or an on/off toggle called in releoadShaders.
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLuint value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glUniform1ui(loc, value);
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, GLfloat value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glProgramUniform1f(m_programId, loc, value);
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::vec2& value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glProgramUniform2f(
			m_programId,
			loc,
			static_cast<GLfloat>(value.x),
			static_cast<GLfloat>(value.y));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::vec3& value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glProgramUniform3f(
			m_programId,
			loc,
			static_cast<GLfloat>(value.x),
			static_cast<GLfloat>(value.y),
			static_cast<GLfloat>(value.z));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::mat3& value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glProgramUniformMatrix3fv(m_programId, loc, 1, GL_FALSE, glm::value_ptr(value));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}
void ShaderProgram::setUniform(const std::string& name, const glm::mat4& value) const {
	GLint loc = uniformLocation(name);
	if (loc != -1) {
		glProgramUniformMatrix4fv(m_programId, loc, 1, GL_FALSE, glm::value_ptr(value));
	}
	else {
		//WARN_LOG << "Uniform does not exist: '" << name << "'";
	}
}

bool ShaderProgram::bindUniformBlock(const std::string& uniformBlockName, GLuint buffer, GLuint uniformBlockBinding) {
	GLuint index = uniformBlockIndex(uniformBlockName);
	if (index == GL_INVALID_INDEX) {
		WARN_LOG << "Uniform Block not found with name " << uniformBlockName;
		return false;
	}
	glBindBufferBase(GL_UNIFORM_BUFFER, uniformBlockBinding, buffer);
	glUniformBlockBinding(m_programId, index, uniformBlockBinding);
	return true;
}

std::vector<std::string> ShaderProgram::getUniformList() const {
	GLint count;
	GLint bufSize;

	glGetProgramiv(m_programId, GL_ACTIVE_UNIFORMS, &count);
	glGetProgramiv(m_programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &bufSize);

	std::vector<GLchar>name(bufSize);

	std::vector<std::string> uniformList(count);

	for (GLuint i = 0; i < static_cast<GLuint>(count); i++) {
		GLint size;
		GLenum type;
		GLsizei length;

		glGetActiveUniform(m_programId, (GLuint)i, bufSize, &length, &size, &type, name.data());
		uniformList[i] = std::string(name.data(), length);
	}

	return uniformList;
}

void ShaderProgram::copy(const ShaderProgram & other)
{
	m_shaderName = other.m_shaderName;
	for (const auto &def : other.m_defines) {
		define(def);
	}
}
