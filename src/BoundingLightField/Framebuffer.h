// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include "Shader.h"

// TODO: take as parameter
constexpr GLsizei MAX_DISPLAY_WIDTH = 2560;
constexpr GLsizei MAX_DISPLAY_HEIGHT = 1600;

typedef struct {
	GLenum format;  // GL_RGBA32F, GL_RGBA32UI...
	GLenum attachement; // GL_COLOR_ATTACHMENT0, ...
} ColorLayerInfo;

class Framebuffer {
public:
	Framebuffer(size_t width = MAX_DISPLAY_WIDTH,
		        size_t height = MAX_DISPLAY_HEIGHT,
		        const std::vector<ColorLayerInfo> & colorLayerInfos = {});
	~Framebuffer();

	void bind() const;

	GLuint depthTexture() const { return m_depthTexture; }
	GLuint colorTexture(size_t i) const { return m_colorTextures[i]; }

private:
	void init();
	void destroy();

private:
	GLsizei m_width, m_height;
	std::vector<ColorLayerInfo> m_colorLayerInfos;

	GLuint m_framebufferId;
	std::vector<GLuint> m_colorTextures;
	GLuint m_depthTexture;
};

