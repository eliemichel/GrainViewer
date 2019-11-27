// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "jsonutils.h"

#include "rapidjson/document.h"

bool jrString(const rapidjson::Value& json, const std::string & key, std::string & target, const std::string & parentName) {
	if (!json.HasMember(key.c_str()) || !json[key.c_str()].IsString()) {
		ERR_LOG << parentName << " must contain a '" << key << "' string field";
		return false;
	}
	target = json[key.c_str()].GetString();
	return true;
}
