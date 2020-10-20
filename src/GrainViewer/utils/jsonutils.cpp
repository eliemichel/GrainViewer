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
