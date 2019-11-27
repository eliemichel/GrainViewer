// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 - 2019 Élie Michel.
// **************************************************

#include <iostream>
#include <sstream>
#include <vector>
#include <utility>
#include <regex>

#include <glm/gtc/type_ptr.hpp>

#include "utils/fileutils.h"
#include "utils/strutils.h"
#include "Logger.h"
#include "ShaderPreprocessor.h"
#include "Shader.h"

Shader::Shader(GLenum shaderType)
    : m_shaderId(0)
{
	m_shaderId = glCreateShader(shaderType);
}


Shader::~Shader() {
    glDeleteShader(m_shaderId);
}

bool Shader::load(const std::string &filename, const std::vector<std::string> & defines, const std::map<std::string, std::string> & snippets) {
	ShaderPreprocessor preprocessor;

	if (!preprocessor.load(filename, defines, snippets)) {
		return false;
	}

	std::vector<GLchar> buf;
	preprocessor.source(buf);
	const GLchar *source = &buf[0];
    glShaderSource(m_shaderId, 1, &source, 0);

#if _DEBUG
	m_preprocessor = preprocessor;
#endif

	return true;
}


bool Shader::check(const std::string & name) const {
    int ok;
    
    glGetShaderiv(m_shaderId, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        int len;
        glGetShaderiv(m_shaderId, GL_INFO_LOG_LENGTH, &len);
        char *log = new char[len];
        glGetShaderInfoLog(m_shaderId, len, &len, log);

		ERR_LOG << "Unable to compile " << name << ". OpenGL returned:";
		if (len == 0) {
			ERR_LOG << "(Driver reported no error message)";
		}
		else {
#ifdef _DEBUG
			// Example of error message:
			// 0(158) : error C0000: syntax error, unexpected '/' at token "/"
			// another example: (intel graphics)
			// ERROR: 0:506: '=' : syntax error syntax error
			std::regex errorPatternA(R"(^(\d+)\((\d+)\) (.*)$)");
			std::regex errorPatternB(R"(^ERROR: (\d+):(\d+): (.*)$)");

			int line = 0;
			std::string msg = "";
			std::string s(log);
			std::smatch match;
			if (regex_search(s, match, errorPatternA)) {
				line = stoi(match[2].str());
				msg = match[3].str();
			}
			else if (regex_search(s, match, errorPatternB)) {
				line = stoi(match[2].str());
				msg = match[3].str();
			}
			ERR_LOG << msg;
			m_preprocessor.logTraceback(static_cast<size_t>(line));
#else // _DEBUG
			ERR_LOG << log;
#endif // _DEBUG
		}

		delete[] log;
    }

    return ok;
}
