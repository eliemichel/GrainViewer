// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <regex>
#include <algorithm>
#include <string>

#include "utils/fileutils.h"
#include "Logger.h"

using namespace std;

string baseDir(const string & path) {
	size_t pos = path.find_last_of(PATH_DELIM);
	return pos != string::npos ? path.substr(0, pos) : "";
}

string shortFileName(const string& path) {
	size_t pos = path.find_last_of(PATH_DELIM);
	return pos != string::npos ? path.substr(pos + 1) : path;
}

string fixPath(const string & path) {
	string p = path;
	p = replaceAll(p, "/", PATH_DELIM);
	p = replaceAll(p, "\\", PATH_DELIM);
	return p;
}

bool isFile(const string & filename) {
	FILE *file = fopen(filename.c_str(), "r");
	if (file) {
		fclose(file);
		return true;
	}
	return false;
}

bool isAbsolutePath(const string & path) {
#ifdef _WIN32
	return path.length() >= 2 && path[1] == ':';
#else
	return path.length() >= 1 && path[0] == '/';
#endif
}

string canonicalPath(const string & path) {
	std::string out = path;
	regex r(string() + PATH_DELIM_ESCAPED + "[^" + PATH_DELIM_ESCAPED + "]+" + PATH_DELIM_ESCAPED + "\\.\\.");
	while (regex_search(out, r)) {
		out = regex_replace(out, r, "", regex_constants::format_first_only);
	}
	return out;
}

string resolvePath(const string & path, const string & basePath) {
	if (isAbsolutePath(path)) {
		return path;
	} else {
		return canonicalPath(joinPath(basePath, path));
	}
}
