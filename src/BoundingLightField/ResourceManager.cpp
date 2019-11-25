// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <glad/glad.h>

#include "utils/strutils.h"
#include "utils/fileutils.h"

#include <SOIL.h>
#include <tinyexr.h>

#include "GlTexture.h"
#include "Logger.h"
#include "ResourceManager.h"

std::string ResourceManager::s_shareDir = SHARE_DIR;
std::string ResourceManager::s_resourceRoot = SHARE_DIR;
std::vector<std::string> ResourceManager::s_shaderPath{ std::string() + SHARE_DIR + PATH_DELIM + "shaders" };

void ResourceManager::setShareDir(const std::string & path) {
	s_shareDir = path;
}

std::string ResourceManager::shareDir() {
	return s_shareDir;
}

void ResourceManager::setResourceRoot(const std::string & path) {
	s_resourceRoot = path;
}

std::string ResourceManager::resourceRoot() {
	return s_resourceRoot;
}

std::string ResourceManager::resolveResourcePath(const std::string & uri) {
	if (startsWith(uri, "res://")) {
		return resolvePath(uri.substr(6), shareDir());
	} else {
		return resolvePath(uri, resourceRoot());
	}
}

std::string ResourceManager::shaderFullPath(const std::string& shaderName, GLenum type) {
	// TODO: Implement a fallback mechanism using all elements of s_shaderPath

	switch (type) {
	case GL_VERTEX_SHADER:
		return fixPath(joinPath(s_shaderPath[0], shaderName + ".vert.glsl"));
	case GL_GEOMETRY_SHADER:
		return fixPath(joinPath(s_shaderPath[0], shaderName + ".geo.glsl"));
	case GL_FRAGMENT_SHADER:
		return fixPath(joinPath(s_shaderPath[0], shaderName + ".frag.glsl"));
	default:
		return "";
	}
}

std::vector<std::string> ResourceManager::allShaderFullPaths(const std::string& shaderName) {
	return {
		shaderFullPath(shaderName, GL_VERTEX_SHADER),
		shaderFullPath(shaderName, GL_GEOMETRY_SHADER),
		shaderFullPath(shaderName, GL_FRAGMENT_SHADER)
	};
}

///////////////////////////////////////////////////////////////////////////////
// Loading procedures
///////////////////////////////////////////////////////////////////////////////

using namespace std;

std::unique_ptr<GlTexture> ResourceManager::loadTexture(const std::string & filename, GLsizei levels)
{
	fs::path fullFilename = resolveResourcePath(filename);

	LOG << "Loading texture " << fullFilename << "...";

	std::unique_ptr<GlTexture> tex;

	if (fullFilename.extension() == ".exr") {
		tex = loadTextureTinyExr(fullFilename, levels);
	}
	else {
		tex = loadTextureSOIL(fullFilename, levels);
	}

	tex->setWrapMode(GL_REPEAT);

	// TODO
	/*
	if (GL_EXT_texture_filter_anisotropic) {
		GLfloat maxi;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxi);
		glTextureParameterf(tex, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxi);
	}
	*/

	tex->generateMipmap();

	return tex;
}

std::unique_ptr<GlTexture> ResourceManager::loadTextureStack(const string & textureDirectory) {
	vector<fs::path> textureFilenames;

	fs::directory_iterator dir(textureDirectory);
	if (dir == fs::directory_iterator())  {
		ERR_LOG << "Error openning texture directory " << textureDirectory;
		return nullptr;
	}

	for (auto& p : dir) {
		if (p.is_regular_file()) {
			textureFilenames.push_back(p.path());
		}
	}

	if (textureFilenames.empty()) {
		ERR_LOG << "Empty texture directory: " << textureDirectory;
		return nullptr;
	}
	DEBUG_LOG << "Found " << textureFilenames.size() << " textures in " << textureDirectory;

	sort(textureFilenames.begin(), textureFilenames.end());

	GLsizei stackSize = static_cast<GLsizei>(textureFilenames.size());
	if (stackSize > GL_MAX_ARRAY_TEXTURE_LAYERS) {
		ERR_LOG << "Number of textures higher than the hardware limit GL_MAX_ARRAY_TEXTURE_LAYERS";
		return nullptr;
	}

	// Init 3D texture
	int imageWidth = 0, imageHeight = 0;
	if (!ResourceManager::imageDimensions(textureFilenames[0], imageWidth, imageHeight)) {
		ERR_LOG << "Could not load texture files.";
		return false;
	}
	GLsizei width = static_cast<GLsizei>(imageWidth);
	GLsizei height = static_cast<GLsizei>(imageHeight);
	DEBUG_LOG << "Allocating texture array of size " << imageWidth << "x" << imageHeight << "x" << stackSize << endl;
	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	tex->setWrapMode(GL_CLAMP_TO_EDGE);
	tex->storage(1, GL_RGBA8, width, height, stackSize);

	// Read all layers
	for (size_t i = 0; i < textureFilenames.size(); ++i) {
		if (!ResourceManager::loadTextureSubData(*tex, textureFilenames[i], static_cast<GLint>(i), width, height)) {
			return false;
		}
	}

	tex->generateMipmap();

	return tex;
}


bool ResourceManager::imageDimensions(const fs::path & filepath, int & width, int & height, Rotation rotation) {
	if (filepath.extension() == ".exr") {
		return imageDimensionsTinyExr(filepath, width, height, rotation);
	}
	else {
		return imageDimensionsSOIL(filepath, width, height, rotation);
	}
}


bool ResourceManager::loadTextureSubData(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei width, GLsizei height, Rotation rotation) {
	if (filepath.extension() == ".exr") {
		return loadTextureSubDataTinyExr(texture, filepath, zoffset, width, height, rotation);
	}
	else {
		return loadTextureSubDataSOIL(texture, filepath, zoffset, width, height, rotation);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Private static utility
///////////////////////////////////////////////////////////////////////////////

template<typename T>
static T * rotateImage(T *image, size_t width, size_t height, size_t nbChannels, size_t *rotatedWidth, size_t *rotatedHeight, ResourceManager::Rotation angle) {
	T *rotated_image;
	if (angle == ResourceManager::ROTATION90 || angle == ResourceManager::ROTATION270) {
		*rotatedWidth = height;
		*rotatedHeight = width;
	}
	else {
		*rotatedWidth = width;
		*rotatedHeight = height;
	}
	rotated_image = new T[*rotatedWidth * *rotatedHeight * nbChannels];
	for (size_t u = 0; u < *rotatedWidth; ++u) {
		for (size_t v = 0; v < *rotatedHeight; ++v) {
			size_t readOffset, writeOffset = v * *rotatedWidth + u;
			switch (angle) {
			case ResourceManager::ROTATION0:
				readOffset = v * width + u;
				break;

			case ResourceManager::ROTATION90:
				readOffset = u * width + (height - v - 1);
				break;

			case ResourceManager::ROTATION180:
				readOffset = (width - v - 1) * width + (height - u - 1);
				break;

			case ResourceManager::ROTATION270:
				readOffset = (width - u - 1) * width + v;
				break;
			}
			for (size_t k = 0; k < nbChannels; ++k) {
				rotated_image[writeOffset * nbChannels + k] = image[readOffset * nbChannels + k];
			}
		}
	}
	return rotated_image;
}

///////////////////////////////////////////////////////////////////////////////
// Private loading procedures
///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<GlTexture> ResourceManager::loadTextureSOIL(const fs::path & filepath, GLsizei levels)
{
	// Load from file
	int width, height;
	unsigned char *image;
	image = SOIL_load_image(filepath.string().c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: " << filepath;
		return nullptr;
	}

	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	tex->storage(levels, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	tex->subImage(0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, image);

	SOIL_free_image_data(image);

	return tex;
}

std::unique_ptr<GlTexture> ResourceManager::loadTextureTinyExr(const fs::path & filepath, GLsizei levels)
{
	// Load from file
	int width, height;
	float *image;
	const char *err;
	int ret = LoadEXR(&image, &width, &height, filepath.string().c_str(), &err);

	if (ret != 0) {
		WARN_LOG << "Unable to load texture from file: " << filepath;
		LOG << "TinyExr returned: " << err;
		return nullptr;
	}

	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	tex->storage(levels, GL_RGBA16F, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	tex->subImage(0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_FLOAT, image);

	free(image);

	return tex;
}

bool ResourceManager::imageDimensionsSOIL(const fs::path & filepath, int & width, int & height, Rotation rotation) {
	unsigned char *image = NULL;
	image = SOIL_load_image(filepath.string().c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file '" << filepath << "' with SOIL: " << SOIL_last_result();
		return false;
	}
	else {
		SOIL_free_image_data(image);
		if (rotation == ROTATION0 || rotation == ROTATION270) {
			int tmp = width;
			width = height;
			height = tmp;
		}
		return true;
	}
}

bool ResourceManager::imageDimensionsTinyExr(const fs::path & filepath, int & width, int & height, Rotation rotation) {
	// TODO: EXR allows to read only image header; use this instead of loading the whole file
	float *image;
	const char *err;
	int ret = LoadEXR(&image, &width, &height, filepath.string().c_str(), &err);

	if (ret != 0) {
		WARN_LOG << "Unable to load texture from file: " << filepath;
		LOG << "TinyExr returned: " << err;
		return false;
	}
	else {
		free(image);
		if (rotation == ROTATION0 || rotation == ROTATION270) {
			int tmp = width;
			width = height;
			height = tmp;
		}
		return true;
	}
}


bool ResourceManager::loadTextureSubDataSOIL(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei width, GLsizei height, Rotation rotation) {
	int imageWidth = 0, imageHeight = 0;
	unsigned char* image = SOIL_load_image(filepath.string().c_str(), &imageWidth, &imageHeight, 0, SOIL_LOAD_RGBA);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file '" << filepath << "' with SOIL: " << SOIL_last_result();
	}

	if (rotation != ROTATION0) {
		size_t rotatedWidth, rotatedHeight;
		unsigned char* rotatedImage = rotateImage(image, static_cast<size_t>(imageWidth), static_cast<size_t>(imageHeight), 4, &rotatedWidth, &rotatedHeight, rotation);
		SOIL_free_image_data(image);
		image = rotatedImage;
		imageWidth = static_cast<int>(rotatedWidth);
		imageHeight = static_cast<int>(rotatedHeight);
	}

	if (imageWidth != width || imageHeight != height) {
		ERR_LOG << "Error: texture array slices must all have the same dimensions.";
		LOG << "Slice #" << (zoffset + 1) << " has dimensions " << imageWidth << "x" << height
			<< " but " << width << "x" << height << " was expected"
			<< " (in file " << filepath << ")." << endl;
		return false;
	}
	texture.subImage(0, 0, 0, zoffset, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, image);

	if (rotation != ROTATION0) {
		free(image);
	}
	else {
		SOIL_free_image_data(image);
	}
	return true;
}


bool ResourceManager::loadTextureSubDataTinyExr(GlTexture & texture, const fs::path & filepath, GLint zoffset, GLsizei width, GLsizei height, Rotation rotation) {
	int imageWidth, imageHeight;
	float *image;
	const char *err;
	int ret = LoadEXR(&image, &imageWidth, &imageHeight, filepath.string().c_str(), &err);

	if (ret != 0) {
		WARN_LOG << "Unable to load texture from file: " << filepath;
		LOG << "TinyExr returned: " << err;
		return false;
	}

	if (rotation != ROTATION0) {
		size_t rotatedWidth, rotatedHeight;
		float* rotatedImage = rotateImage(image, static_cast<size_t>(imageWidth), static_cast<size_t>(imageHeight), 4, &rotatedWidth, &rotatedHeight, rotation);
		free(image);
		image = rotatedImage;
		imageWidth = static_cast<int>(rotatedWidth);
		imageHeight = static_cast<int>(rotatedHeight);
	}

	if (imageWidth != width || imageHeight != height) {
		ERR_LOG << "Error: texture array slices must all have the same dimensions.";
		LOG << "Slice #" << (zoffset + 1) << " has dimensions " << imageWidth << "x" << height
			<< " but " << width << "x" << height << " was expected." << endl;
		return false;
	}

	texture.subImage(0, 0, 0, zoffset, width, height, 1, GL_RGBA, GL_FLOAT, image);
	free(image);
	return true;
}
