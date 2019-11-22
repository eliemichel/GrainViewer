// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "utils/strutils.h"
#include "utils/fileutils.h"

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
