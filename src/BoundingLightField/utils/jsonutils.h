// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

#include "Logger.h"

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <string>
#include <vector>

#include "utils/tplutils.h"

typedef rapidjson::Writer<rapidjson::OStreamWrapper> JsonWriter;

bool jrString(const rapidjson::Value& json, const std::string & key, std::string & target, const std::string & parentName);

template<typename T> static inline bool _read(T & target, const rapidjson::Value& json) {
	return json.IsObject() && target.readJson(json);
}
template<> static inline bool _read(float & target, const rapidjson::Value& json) {
	return json.IsNumber() && ((target = json.GetFloat()) || true);
}
template<> static inline bool _read(int & target, const rapidjson::Value& json) {
	return json.IsInt() && ((target = json.GetInt()) || true);
}
template<> static inline bool _read(bool & target, const rapidjson::Value& json) {
	return json.IsBool() && ((target = json.GetBool()) || true);
}
template<> static inline bool _read(std::string & target, const rapidjson::Value& json) {
	if (!json.IsString()) return false;
	target = json.GetString();
	return true;
}

template <typename T>
inline bool jrOption(const rapidjson::Value& json, const std::string & key, T & target, const T& defaultValue = T()) {
	if (!json.HasMember(key.c_str())) {
		target = defaultValue;
		return false;
	}

	const rapidjson::Value& d = json[key.c_str()];
	if (!_read(target, d)) {
		WARN_LOG << "Ignoring '" << key << "' (expected type " << TypeName<T>::Get() << ")";
		target = defaultValue;
		return false;
	}
	return true;
}

template<typename T>
inline bool jrArray(const rapidjson::Value& json, const std::string & key, std::vector<T> & target, int size = -1) {
	if (json.HasMember(key.c_str())) {
		const rapidjson::Value& d = json[key.c_str()];
		if (!d.IsArray()) {
			WARN_LOG << "Ignoring '" << key << "' (expected an array)";
			return false;
		}
		if (size != -1 && static_cast<int>(d.Size()) != size) {
			WARN_LOG << "Ignoring '" << key << "' (expected an array of size " << size << ")";
			return false;
		}
		target.resize(static_cast<size_t>(d.Size()));
		for (rapidjson::SizeType i = 0; i < d.Size(); i++) {
			const rapidjson::Value& dd = d[i];
			T & item = target[static_cast<size_t>(i)];
			if (!_read(item, dd)) {
				WARN_LOG << "Ignoring '" << key << "' (error while parsing item #" << i << " as type " << TypeName<T>::Get() << ")";
				target.resize(0);
				return false;
			}
		}
	}
	return true;
}


template<typename T> static inline void _write(const T & v, JsonWriter & writer) {
	v.writeJson(writer);
}
template<> static inline void _write(const float & v, JsonWriter & writer) {
	writer.Double(v);
}
template<> static inline void _write(const int & v, JsonWriter & writer) {
	writer.Int(v);
}
template<> static inline void _write(const bool & v, JsonWriter & writer) {
	writer.Bool(v);
}
template<> static inline void _write(const std::string & v, JsonWriter & writer) {
	writer.String(v.c_str());
}
static inline void _write(const char* v, JsonWriter & writer) {
	writer.String(v);
}

template<typename T>
inline void jwArray(JsonWriter & writer, const std::string & key, const std::vector<T> & values) {
	writer.Key(key.c_str());
	writer.StartArray();
	for (const auto& v : values) {
		_write(v, writer);
	}
	writer.EndArray();
}

template<typename T>
inline void jw(JsonWriter & writer, const std::string & key, const T & v) {
	writer.Key(key.c_str());
	_write(v, writer);
}


// Utility preprocessor routines to be used with canonical variable names
#define JREAD_DEFAULT(x, d) jrOption(json, #x, m_ ## x, d)
#define JREAD(x) jrOption(json, #x, m_ ## x)
#define JREAD_ARRAY(x, d) jrArray(json, #x, m_ ## x, d)
#define JREAD_LIST(x) jrArray(json, #x, m_ ## x)
#define JWRITE(x) jw(writer, #x, m_ ## x)
