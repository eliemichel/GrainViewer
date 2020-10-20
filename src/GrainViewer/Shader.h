// **************************************************
// Author : Élie Michel <elie.michel@telecom-paris.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 - 2019 Élie Michel.
// **************************************************

#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>

#ifndef NDEBUG
#include "ShaderPreprocessor.h"
#endif

/**
 * Utility class providing an OO API to OpenGL shaders
 */
class Shader {
public:
    Shader(GLenum shaderType = 0);
    ~Shader();

    /**
     * Load file into the shader
     * @param filename Shader file
     */
    bool load(const std::string &filename, const std::vector<std::string> & defines = {}, const std::map<std::string, std::string> & snippets = {});

    /**
     * Compile the shader
     */
    inline void compile() { glCompileShader(m_shaderId); }

    /**
     * Check that the shader has been successfully compiled
     * @param name Name displayed in error message
     * @param success status
     */
    bool check(const std::string & name = "shader") const;

    inline GLuint shaderId() const { return m_shaderId; }

private:
    GLuint m_shaderId;

#ifndef NDEBUG
	// Keep shader source
	ShaderPreprocessor m_preprocessor;
#endif
};
