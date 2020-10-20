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

#include <OpenGL>

#include "ResourceManager.h"
#include "EnvironmentVariables.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"
#include "GlTexture.h"
#include "Logger.h"

#include <glm/glm.hpp>
#include <png.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tinyexr.h>
#include <TinyPngOut.hpp>

#include <cmath>
#include <fstream>
#include <algorithm>
#include <filesystem>
namespace fs = std::filesystem;

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
	std::string evaluated = EnvironmentVariables::Eval(uri);
	if (startsWith(evaluated, "res://")) {
		return resolvePath(evaluated.substr(6), shareDir());
	} else {
		return resolvePath(evaluated, resourceRoot());
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
	case GL_COMPUTE_SHADER:
		return fixPath(joinPath(s_shaderPath[0], shaderName + ".comp.glsl"));
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

	if (tex) {
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
	}

	return tex;
}

std::unique_ptr<GlTexture> ResourceManager::loadTextureStack(const string & textureDirectory, int levels) {
	vector<fs::path> textureFilenames;

	std::string fullTextureDirectory = ResourceManager::resolveResourcePath(textureDirectory);

	if (!fs::is_directory(fullTextureDirectory)) {
		ERR_LOG << "Texture directory does not exist or is not a directory: " << fullTextureDirectory;
		return nullptr;
	}

	fs::directory_iterator dir(fullTextureDirectory);
	if (dir == fs::directory_iterator())  {
		ERR_LOG << "Error openning texture directory " << fullTextureDirectory;
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
		return nullptr;
	}
	GLsizei width = static_cast<GLsizei>(imageWidth);
	GLsizei height = static_cast<GLsizei>(imageHeight);
	DEBUG_LOG << "Allocating texture array of size " << imageWidth << "x" << imageHeight << "x" << stackSize;
	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D_ARRAY);
	tex->setWrapMode(GL_CLAMP_TO_EDGE);
	tex->storage(levels, GL_RGBA8, width, height, stackSize);

	// Read all layers
	for (size_t i = 0; i < textureFilenames.size(); ++i) {
		if (!ResourceManager::loadTextureSubData(*tex, textureFilenames[i], static_cast<GLint>(i), width, height)) {
			return nullptr;
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
// File output
///////////////////////////////////////////////////////////////////////////////

bool ResourceManager::saveImage(const std::string & filename, int width, int height, const void *data)
{
	fs::create_directories(fs::path(filename).parent_path());
	std::ofstream out(filename, std::ios::binary);
	if (!out.is_open()) {
		WARN_LOG << "Could not open file '" << filename << "'";
		return false;
	}
	TinyPngOut pngout(static_cast<uint32_t>(width), static_cast<uint32_t>(height), out);
	pngout.write(static_cast<const uint8_t*>(data), static_cast<size_t>(width * height));
	return true;
}

bool ResourceManager::saveImage_libpng(const std::string & filename, int width, int height, void *data)
{
	FILE *fp;
#ifdef _WIN32
	fopen_s(&fp, filename.c_str(), "wb");
#else // _WIN32
	fp = fopen(filename.c_str(), "wb");
#endif // _WIN32
	if (!fp) {
		return false;
	}

	png_structp png_ptr = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		(png_voidp)nullptr,
		[](png_structp png_ptr, png_const_charp c) { ERR_LOG << "PNG error: " << c; },
		[](png_structp png_ptr, png_const_charp c) { WARN_LOG << "PNG warning: " << c; }
	);

	if (!png_ptr) {
		return false;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
		return false;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}

	png_set_check_for_invalid_index(png_ptr, 0);
	png_init_io(png_ptr, fp);

	png_set_IHDR(
		png_ptr,
		info_ptr,
		static_cast<png_uint_32>(width),
		static_cast<png_uint_32>(height),
		8,
		PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT
	);
	png_write_info(png_ptr, info_ptr);

	std::vector<png_bytep> row_pointers(height);
	for (int i = 0; i < height; ++i) {
		row_pointers[i] = static_cast<png_bytep>(data) + 4 * i * width;
	}

	png_write_image(png_ptr, row_pointers.data());
	png_write_end(png_ptr, NULL);

	fclose(fp);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	return true;
}

bool ResourceManager::saveTexture(const std::string & filename, const GlTexture & texture)
{
	GLsizei w = texture.width();
	GLsizei h = texture.height();
	GLsizei d = texture.depth();
	std::vector<uint8_t> pixels(3 * w * h);
	glGetTextureSubImage(texture.raw(), 0, 0, 0, 0, w, h, d, GL_RGB, GL_UNSIGNED_BYTE, w * h * 3 * sizeof(uint8_t), pixels.data());
	return saveImage(filename, w, h, pixels.data());
}

bool ResourceManager::saveTextureStack(const std::string& dirname, const GlTexture& texture, bool vflip)
{
	GLint level = 0;
	GLint d;
	glGetTextureLevelParameteriv(texture.raw(), level, GL_TEXTURE_DEPTH, &d);

	fs::path dir = ResourceManager::resolveResourcePath(dirname);
	if (!fs::is_directory(dir) && !fs::create_directories(dir)) {
		DEBUG_LOG << "Could not create directory " << dirname;
		return false;
	}

	for (GLint slice = 0; slice < d; ++slice) {
		fs::path slicename = dir / string_format("view%04d.png", slice);
		if (!saveTexture_libpng(slicename.string(), texture, level, vflip, slice)) {
			//return false;
		}
	}

	return true;
}

bool ResourceManager::saveTexture_libpng(const std::string & filename, GLuint tex, GLint level, bool vflip, GLint slice)
{
	// Avoid padding
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	stbi_flip_vertically_on_write(vflip ? 1 : 0);

	// Check level
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	if (level < baseLevel || level >= maxLevel) {
		ERR_LOG
			<< "Invalid mipmap level " << level << " for texture " << tex
			<< " (expected in range " << baseLevel << ".." << maxLevel << ")";
		return false;
	}

	GLint w, h, d;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_WIDTH, &w);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_HEIGHT, &h);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_DEPTH, &d);

	if (w == 0 || h == 0) {
		ERR_LOG << "Texture with null size: " << tex;
		return false;
	}

	if (slice >= d) {
		ERR_LOG << "Texture depth " << d << " is lower than the selected slice " << slice;
		return false;
	}

	GLint internalformat;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);

	std::vector<png_byte> pixels;

	switch (internalformat) {
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	{
		GLsizei byteCount = w * h;
		pixels = std::vector<png_byte>(byteCount, 0);
		glGetTextureSubImage(tex, 0, 0, 0, slice, w, h, 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, byteCount * sizeof(png_byte), pixels.data());
		return stbi_write_png(filename.c_str(), w, h, 1, pixels.data(), w) == 0;
	}
	default:
	{
		GLsizei byteCount = 4 * w * h;
		pixels.resize(byteCount);
		glGetTextureSubImage(tex, 0, 0, 0, slice, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, byteCount * sizeof(png_byte), pixels.data());
		return stbi_write_png(filename.c_str(), w, h, 4, pixels.data(), 4 * w) == 0;
	}
	}
}

bool ResourceManager::saveTexture_libpng(const std::string& filename, const GlTexture& texture, GLint level, bool vflip, GLint slice)
{
	return saveTexture_libpng(filename, texture.raw(), level, vflip, slice);
}

bool ResourceManager::saveTexture_tinyexr(const std::string & filename, GLuint tex, GLint level)
{
	// Avoid padding
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	// Check level
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	if (level < baseLevel || level >= maxLevel) {
		ERR_LOG
			<< "Invalid mipmap level " << level << " for texture " << tex
			<< " (expected in range " << baseLevel << ".." << maxLevel << ")";
		return false;
	}

	GLint w, h, d;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_WIDTH, &w);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_HEIGHT, &h);
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_DEPTH, &d);

	if (w == 0 || h == 0) {
		ERR_LOG << "Texture with null size: " << tex;
		return false;
	}

	if (d != 1) {
		WARN_LOG << "Texture depth is clamped to 1 upon saving";
		d = 1;
	}

	GLint internalformat;
	glGetTextureLevelParameteriv(tex, level, GL_TEXTURE_INTERNAL_FORMAT, &internalformat);

	std::vector<png_byte> pixels;

	switch (internalformat) {
	case GL_DEPTH_COMPONENT16:
	case GL_DEPTH_COMPONENT24:
	case GL_DEPTH_COMPONENT32:
	{
		GLsizei pixelCount = w * h * d;
		std::vector<float> pixels = std::vector<float>(pixelCount, 0);
		glGetTextureSubImage(tex, level, 0, 0, 0, w, h, d, GL_DEPTH_COMPONENT, GL_FLOAT, pixelCount * sizeof(float), pixels.data());
		
		EXRHeader header;
		InitEXRHeader(&header);

		EXRImage image;
		InitEXRImage(&image);

		image.num_channels = 1;
		float* image_ptr[1];
		image_ptr[0] = pixels.data();
		image.images = (unsigned char**)image_ptr;
		image.width = w;
		image.height = h;
		header.num_channels = 1;
		header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
		header.channels[0].name[0] = 'R'; header.channels[0].name[1] = '\0';

		header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
		header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
		for (int i = 0; i < header.num_channels; i++) {
			header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
			header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
		}

		const char* err = nullptr;
		int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
		if (ret != TINYEXR_SUCCESS) {
			ERR_LOG << "tinyexr: " << err;
			FreeEXRErrorMessage(err); // free's buffer for an error message
			return false;
		}

		free(header.channels);
		free(header.pixel_types);
		free(header.requested_pixel_types);

		return true;
	}
	default:
	{
		GLsizei pixelCount = w * h * d;
		std::vector<glm::vec4> pixels = std::vector<glm::vec4>(pixelCount);
		glGetTextureSubImage(tex, level, 0, 0, 0, w, h, d, GL_RGBA, GL_FLOAT, pixelCount * sizeof(glm::vec4), pixels.data());
		std::vector<float> red = std::vector<float>(pixelCount);
		std::vector<float> green = std::vector<float>(pixelCount);
		std::vector<float> blue = std::vector<float>(pixelCount);
		std::vector<float> alpha = std::vector<float>(pixelCount);
		// Split RGBARGBARGBA... into R, G, B and A layers
		for (int i = 0; i < pixelCount; i++) {
			red[i] = pixels[i].r;
			green[i] = pixels[i].g;
			blue[i] = pixels[i].b;
			alpha[i] = pixels[i].a;
		}

		EXRHeader header;
		InitEXRHeader(&header);

		EXRImage image;
		InitEXRImage(&image);

		image.num_channels = 4;
		float* image_ptr[4];
		image_ptr[0] = alpha.data();
		image_ptr[1] = blue.data();
		image_ptr[2] = green.data();
		image_ptr[3] = red.data();
		image.images = (unsigned char**)image_ptr;
		image.width = w;
		image.height = h;
		header.num_channels = 4;
		header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
		header.channels[0].name[0] = 'A'; header.channels[0].name[1] = '\0';
		header.channels[1].name[0] = 'B'; header.channels[1].name[1] = '\0';
		header.channels[2].name[0] = 'G'; header.channels[2].name[1] = '\0';
		header.channels[3].name[0] = 'R'; header.channels[3].name[1] = '\0';

		header.pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
		header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
		for (int i = 0; i < header.num_channels; i++) {
			header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
			header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of output image to be stored in .EXR
		}

		const char* err = nullptr;
		int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
		if (ret != TINYEXR_SUCCESS) {
			ERR_LOG << "tinyexr: " << err;
			FreeEXRErrorMessage(err); // free's buffer for an error message
			return false;
		}

		free(header.channels);
		free(header.pixel_types);
		free(header.requested_pixel_types);

		return true;
	}
	}
}

bool ResourceManager::saveTexture_tinyexr(const std::string & filename, const GlTexture& texture, GLint level)
{
	return saveTexture_tinyexr(filename, texture.raw(), level);
}

bool ResourceManager::saveTextureMipMaps(const std::string& prefix, GLuint tex)
{
	GLint baseLevel, maxLevel;
	glGetTextureParameteriv(tex, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTextureParameteriv(tex, GL_TEXTURE_MAX_LEVEL, &maxLevel);
	for (GLint level = baseLevel; level < maxLevel; ++level) {
		if (!ResourceManager::saveTexture_libpng(prefix + std::to_string(level) + ".png", tex, level)) {
			return false;
		}
	}
	return true;
}

bool ResourceManager::saveTextureMipMaps(const std::string& prefix, const GlTexture& texture)
{
	return saveTextureMipMaps(prefix, texture.raw());
}

///////////////////////////////////////////////////////////////////////////////
// Private loading procedures
///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<GlTexture> ResourceManager::loadTextureSOIL(const fs::path & filepath, GLsizei levels)
{
	// Load from file
	int width, height, channels;
	unsigned char *image;
	image = stbi_load(filepath.string().c_str(), &width, &height, &channels, 4);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return nullptr;
	}

	if (levels == 0) {
		levels = static_cast<GLsizei>(1 + floor(log2(max(width, height))));
	}

	auto tex = std::make_unique<GlTexture>(GL_TEXTURE_2D);
	tex->storage(levels, GL_RGBA8, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	tex->subImage(0, 0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height), GL_RGBA, GL_UNSIGNED_BYTE, image);

	stbi_image_free(image);

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
	int channels;
	image = stbi_load(filepath.string().c_str(), &width, &height, &channels, 0);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return false;
	}
	else {
		stbi_image_free(image);
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
	int imageWidth = 0, imageHeight = 0, channels;
	unsigned char *image = stbi_load(filepath.string().c_str(), &imageWidth, &imageHeight, &channels, 4);
	if (NULL == image) {
		WARN_LOG << "Unable to load texture file: '" << filepath << "' with stb_image: " << stbi_failure_reason();
		return false;
	}

	if (rotation != ROTATION0) {
		size_t rotatedWidth, rotatedHeight;
		unsigned char* rotatedImage = rotateImage(image, static_cast<size_t>(imageWidth), static_cast<size_t>(imageHeight), 4, &rotatedWidth, &rotatedHeight, rotation);
		stbi_image_free(image);
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
		stbi_image_free(image);
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
