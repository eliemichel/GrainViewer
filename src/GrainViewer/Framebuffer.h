// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include "Shader.h"

#include <rapidjson/document.h>

struct ColorLayerInfo {
	GLenum format;  // GL_RGBA32F, GL_RGBA32UI...
	GLenum attachement; // GL_COLOR_ATTACHMENT0, ...

	bool deserialize(const rapidjson::Value& json);
};

class Framebuffer {
public:
	/**
	 * @param mipmapDepthBuffer Turn it on when implementing Hierarchical Z Buffer
	 */
	Framebuffer(size_t width,
		        size_t height,
		        const std::vector<ColorLayerInfo> & colorLayerInfos = {},
		        bool mipmapDepthBuffer = false);
	~Framebuffer();

	void bind() const;

	GLuint depthTexture() const { return m_depthTexture; }
	GLuint colorTexture(size_t i) const { return m_colorTextures[i]; }
	size_t colorTextureCount() const { return m_colorTextures.size(); }

	GLuint raw() const { return m_framebufferId; }

	GLsizei width() const { return m_width; }
	GLsizei height() const { return m_height; }

	/**
	 * Use this with caution, it reallocates video memory
	 */
	void setResolution(size_t width, size_t height);

	/**
	 * Number of mipmap levels in depth buffer
	 */
	GLsizei depthLevels() const;

	void deactivateColorAttachments();
	void activateColorAttachments();

private:
	void init();
	void destroy();

private:
	GLsizei m_width, m_height;
	std::vector<ColorLayerInfo> m_colorLayerInfos;

	GLsizei m_depthLevels;

	GLuint m_framebufferId;
	std::vector<GLuint> m_colorTextures;
	GLuint m_depthTexture;

	// Allocated only when the framebuffer is saved to file, assuming that if
	// it happens once, it is likely to hapen again
	std::vector<uint8_t> m_pixels;
};

