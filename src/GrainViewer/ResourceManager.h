/**
 * This file is part of GrainViewer
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

#include <OpenGL>

#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

class GlTexture;

/**
 * Class handling image I/O and external resource path resolution
 * (TODO: maybe these two parts should get split by the way)
 * There are a few alternatives for load/save library, mostly for
 * backward compatibility, may require some clean up (and the use of
 * stb_image in lieu of libpng?)
 */
class ResourceManager {
public:
	enum Rotation {
		ROTATION0,
		ROTATION90,
		ROTATION180,
		ROTATION270
	};

public:
	static void setShareDir(const std::string & path);
	static std::string shareDir();

	static void setResourceRoot(const std::string & path);
	static std::string resourceRoot();

	/**
	 * Transform a Augen resource identifier into an absolute file path
	 */
	static std::string resolveResourcePath(const std::string & uri);

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

public:
	/**
	 * Load a regular texture
	 */
	static std::unique_ptr<GlTexture> loadTexture(const std::string & filename, GLsizei levels = 0);

	/**
	 * Load of stack of textures as a GL_TEXTURE_2D_ARRAY
	 */
	static std::unique_ptr<GlTexture> loadTextureStack(const std::string & textureDirectory, int levels = 0);

	/**
	 * Get the width and height of an image
	 */
	static bool imageDimensions(const fs::path & filepath, int & width, int & height, Rotation rotation = ROTATION0);

	/**
	 * Add data in an already existing texture
	 * (TODO: this may belong on GlTexture)
	 */
	static bool loadTextureSubData(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei refWidth, GLsizei refHeight, Rotation rotation = ROTATION0);

public:
	// File output
	static bool saveImage(const std::string & filename, int width, int height, int channels, const void *data, bool vflip = false);

	static bool saveTextureStack(const std::string& dirname, const GlTexture& texture, bool vflip = false);
	static bool saveTexture(const std::string & filename, GLuint tex, GLint level = 0, bool vflip = false, GLint slice = 0);
	static bool saveTexture(const std::string& filename, const GlTexture& texture, GLint level = 0, bool vflip = false, GLint slice = 0);

	static bool saveTexture_tinyexr(const std::string & filename, GLuint tex, GLint level = 0);
	static bool saveTexture_tinyexr(const std::string & filename, const GlTexture& texture, GLint level = 0);

	// Save all mipmap levels in prefixXX.png
	static bool saveTextureMipMaps(const std::string& prefix, GLuint tex);
	static bool saveTextureMipMaps(const std::string& prefix, const GlTexture& texture);

private:
	static std::unique_ptr<GlTexture> loadTextureSOIL(const fs::path & filepath, GLsizei levels);
	static std::unique_ptr<GlTexture> loadTextureTinyExr(const fs::path & filepath, GLsizei levels);

	static bool imageDimensionsSOIL(const fs::path & filepath, int & width, int & height, Rotation rotation = ROTATION0);
	static bool imageDimensionsTinyExr(const fs::path & filepath, int & width, int & height, Rotation rotation = ROTATION0);

	static bool loadTextureSubDataSOIL(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei refWidth, GLsizei refHeight, Rotation rotation = ROTATION0);
	static bool loadTextureSubDataTinyExr(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei refWidth, GLsizei refHeight, Rotation rotation = ROTATION0);

private:
	static std::string s_shareDir;
	static std::string s_resourceRoot;
	static std::vector<std::string> s_shaderPath;
};

