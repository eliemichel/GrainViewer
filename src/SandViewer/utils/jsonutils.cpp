// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "jsonutils.h"
#include "fileutils.h"
#include "ResourceManager.h"

#include "rapidjson/document.h"

#include <fstream>

bool openJson(const std::string& filename, rapidjson::Document& d) {
	std::ifstream in(filename);

	if (!in.is_open()) {
		ERR_LOG << filename << ": Unable to read";
		return false;
	}

	LOG << "Loading scene from JSON file " << filename << "...";
	ResourceManager::setResourceRoot(baseDir(filename));

	std::string json;
	std::string line;
	while (getline(in, line)) {
		json += line;
	}

	if (d.Parse(json.c_str()).HasParseError()) {
		ERR_LOG << "rapidjson: " << d.GetParseError();
		ERR_LOG << "Parse error while reading JSON file " << filename;
		return false;
	}

	return true;
}

bool jrString(const rapidjson::Value& json, const std::string & key, std::string & target, const std::string & parentName) {
	if (!json.HasMember(key.c_str()) || !json[key.c_str()].IsString()) {
		ERR_LOG << parentName << " must contain a '" << key << "' string field";
		return false;
	}
	target = json[key.c_str()].GetString();
	return true;
}
