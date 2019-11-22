// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <string>

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

private:
	static std::string s_shareDir;
	static std::string s_resourceRoot;
};

