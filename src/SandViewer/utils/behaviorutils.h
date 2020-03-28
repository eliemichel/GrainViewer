#pragma once

#include <string>
#include <refl.hpp>
#include "utils/jsonutils.h"
#include "ShaderProgram.h"

/**
 * Automatically deserialize properties using reflection.
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoDeserialize(const rapidjson::Value& json, T& properties) {
	// Deserialize properties using reflection
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		// bool, float, etc. properties (whatever is supported by jrOption
		if constexpr (
			std::is_same_v<type, bool>
			|| std::is_same_v<type, float>)
		{
			jrOption(json, std::string(member.name), member(properties), member(properties));
		}
		// Enum properties
		else if constexpr (std::is_enum_v<type>) {
			int value = member(properties);
			jrOption(json, std::string(member.name), value, value);
			member(properties) = static_cast<typename decltype(member)::value_type>(value);
		}
		else {
			ERR_LOG << "Unsupported type for property '" << member.name << "'";
		}
	});
}

/**
 * Automatically bind properties using reflection.
 * The type T must have reflection enabled (see refl-cpp)
 */
template<typename T>
void autoSetUniforms(const ShaderProgram& shader, const T& properties) {
	// Automatically bind properties using reflection
	for_each(refl::reflect(properties).members, [&](auto member) {
		using type = typename decltype(member)::value_type;
		// whatever is supported by setUniform
		if constexpr (
			std::is_same_v<type, bool>
			|| std::is_same_v<type, float>
			|| std::is_same_v<type, glm::vec3>
			|| std::is_enum_v<type>)
		{
			// transform property name "foo" into uniform name "uFoo"
			// (ideally this transformation should be performed at compile time)
			std::string name = "u" + std::string(member.name);
			name[1] = toupper(name[1]);
			shader.setUniform(name, member(properties));
		}
	});
}
