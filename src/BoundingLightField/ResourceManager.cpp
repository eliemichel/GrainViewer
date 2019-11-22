// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "ResourceManager.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"

using namespace std;

string ResourceManager::s_shareDir = "C:\\Elie\\src\\Augen\\share";

void ResourceManager::setShareDir(const string & path) {
	s_shareDir = path;
}

string ResourceManager::shareDir() {
	return s_shareDir;
}

string ResourceManager::s_resourceRoot = "C:\\Elie\\src\\Augen\\share";

void ResourceManager::setResourceRoot(const string & path) {
	s_resourceRoot = path;
}

string ResourceManager::resourceRoot() {
	return s_resourceRoot;
}

string ResourceManager::resolveResourcePath(const string & uri) {
	if (startsWith(uri, "res://")) {
		return resolvePath(uri.substr(6), shareDir());
	} else {
		return resolvePath(uri, resourceRoot());
	}
}
