// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include "strutils.h"

#include <string>
#include <sstream>

#ifdef _WIN32
constexpr auto PATH_DELIM = "\\";
constexpr auto PATH_DELIM_CHAR = '\\';
constexpr auto PATH_DELIM_ESCAPED = "\\\\";
#else
constexpr auto PATH_DELIM = "/";
constexpr auto PATH_DELIM_CHAR = '/';
constexpr auto PATH_DELIM_ESCAPED = "/";
#endif

inline std::string joinPath(const std::string& head) {
	return head;
}

template<typename... Args>
inline std::string joinPath(const std::string& head, Args&&... rest) {
	std::string correctedHead = endsWith(head, PATH_DELIM) ? head.substr(0, head.size() - 1) : head;
	return correctedHead + std::string(PATH_DELIM) + joinPath(rest...);
}

std::string baseDir(const std::string & path);

/// return foo.bar from /some/path/to/foo.bar
std::string shortFileName(const std::string& path);

// Transform both all / and \ into PATH_DELIM
std::string fixPath(const std::string & path);

bool isFile(const std::string & filename);

bool isAbsolutePath(const std::string & path);

/**
 * Return path with no ".."
 */
std::string canonicalPath(const std::string & path);

/**
 * Transform path to absolute path, using base dir as current directory when
 * input path is relative.
 */
std::string resolvePath(const std::string & path, const std::string & basePath);
