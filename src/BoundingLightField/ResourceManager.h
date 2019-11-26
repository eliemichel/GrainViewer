// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <filesystem>
namespace fs = std::filesystem;

class GlTexture;

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

