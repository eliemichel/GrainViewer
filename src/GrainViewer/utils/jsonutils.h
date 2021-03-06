/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#pragma once

#include "Logger.h"
#include "SerializationType.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <memory>
#include <string>
#include <vector>

typedef rapidjson::Writer<rapidjson::OStreamWrapper> JsonWriter;

bool openJson(const std::string& filename, rapidjson::Document& d);

bool jrString(const rapidjson::Value& json, const std::string & key, std::string & target, const std::string & parentName);

template<typename T> inline bool _read(T & target, const rapidjson::Value& json) {
	return /*json.IsObject() &&*/ target.deserialize(json);
}
template<typename T> inline bool _read(std::vector<T>& target, const rapidjson::Value& json) {
	if (!json.IsArray()) return false;
	target.reserve(json.Size());
	for (int k = 0; k < static_cast<int>(json.Size()); ++k) {
		T item;
		if (!_read(item, json[k])) {
			target.resize(0);
			return false;
		}
		target.push_back(std::move(item));
	}
	return true;
}
template<> inline bool _read(float & target, const rapidjson::Value& json) {
	return json.IsNumber() && ((target = json.GetFloat()) || true);
}
template<> inline bool _read(int & target, const rapidjson::Value& json) {
	return json.IsInt() && ((target = json.GetInt()) || true);
}
template<> inline bool _read(bool & target, const rapidjson::Value& json) {
	return json.IsBool() && ((target = json.GetBool()) || true);
}
template<> inline bool _read(std::string & target, const rapidjson::Value& json) {
	if (!json.IsString()) return false;
	target = json.GetString();
	return true;
}
template<> inline bool _read(glm::vec3 & target, const rapidjson::Value& json) {
	if (!json.IsArray() || json.Size() != 3) return false;
	for (int k = 0 ; k < 3 ; ++k) target[k] = json[k].GetFloat();
	return true;
}
template<> inline bool _read(glm::vec4& target, const rapidjson::Value& json) {
	if (!json.IsArray() || json.Size() != 4) return false;
	for (int k = 0; k < 4; ++k) target[k] = json[k].GetFloat();
	return true;
}
template<> inline bool _read(glm::quat & target, const rapidjson::Value& json) {
	if (!json.IsArray() || json.Size() != 4) return false;
	for (int k = 0; k < 4; ++k) target[k] = json[k].GetFloat();
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
		WARN_LOG << "Ignoring '" << key << "' (expected type " << SerializationType<T>::Name() << ")";
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
				WARN_LOG << "Ignoring '" << key << "' (error while parsing item #" << i << " as type " << SerializationType<T>::Name() << ")";
				target.resize(0);
				return false;
			}
		}
	}
	return true;
}


template<typename T> inline void _write(const T & v, JsonWriter & writer) {
	v.writeJson(writer);
}
template<> inline void _write(const float & v, JsonWriter & writer) {
	writer.Double(v);
}
template<> inline void _write(const int & v, JsonWriter & writer) {
	writer.Int(v);
}
template<> inline void _write(const bool & v, JsonWriter & writer) {
	writer.Bool(v);
}
template<> inline void _write(const std::string & v, JsonWriter & writer) {
	writer.String(v.c_str());
}
inline void _write(const char* v, JsonWriter & writer) {
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
#define JREADp(x) jrOption(json, #x, x)
#define JREAD_ARRAY(x, d) jrArray(json, #x, m_ ## x, d)
#define JREAD_LIST(x) jrArray(json, #x, m_ ## x)
#define JWRITE(x) jw(writer, #x, m_ ## x)
#define JWRITEp(x) jw(writer, #x, x)
