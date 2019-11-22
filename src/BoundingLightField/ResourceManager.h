// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

class ResourceManager {
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

private:
	static std::string s_shareDir;
	static std::string s_resourceRoot;
	static std::vector<std::string> s_shaderPath;
};

