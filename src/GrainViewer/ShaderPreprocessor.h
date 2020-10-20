// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 - 2019 Élie Michel.
// **************************************************

#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <string>
#include <vector>
#include <map>

class ShaderPreprocessor {
public:
	/**
	 * Load a shader from a file, and reccursively include #include'd files
	 * You can specity a list of defines to #define where ever
	 * `#include "sys:defines"` is found in the shader.
	 */
	bool load(const std::string & filename, const std::vector<std::string> & defines = {}, const std::map<std::string, std::string> & snippets = {});

	/**
	 * Return a buffer to the pure GLSL source, post-preprocessing, to be fed into
	 * glShaderSource.
	 */
	void source(std::vector<GLchar> & buf) const;

	/**
	 * Log a traceback corresponding to the line `line` in the processed source.
	 */
	void logTraceback(size_t line) const;

private:
	std::vector<std::string> m_lines;

private:
	static bool loadShaderSourceAux(const std::string & filename, const std::vector<std::string> & defines, const std::map<std::string, std::string> & snippets, std::vector<std::string> & lines_accumulator);
};

