/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

#include "Shader.h"

#include <rapidjson/document.h>

struct ColorLayerInfo {
	GLenum format;  // GL_RGBA32F, GL_RGBA32UI...
	GLenum attachement; // GL_COLOR_ATTACHMENT0, ...

	bool deserialize(const rapidjson::Value& json);
};

/**
 * Deprecated, try and use Framebuffer2 instead
 */
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
	// it happens once, it is likely to happen again
	std::vector<uint8_t> m_pixels;
};

