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

#include "ShaderProgram.h"

#include <rapidjson/document.h>

#include <map>
#include <string>
#include <memory>

class ShaderPool {
public:
	static void AddShader(
		const std::string& shaderName,
		const std::string& baseFile,
		ShaderProgram::ShaderProgramType type = ShaderProgram::RenderShader,
		const std::vector<std::string>& defines = {},
		const std::map<std::string, std::string> & snippets = {});

	/**
	 * Copy a base shader and add an extra define. If the define is already
	 * activated in the original shader, this creates an alias pointing to the
	 * previous shader, not a deep copy. This has no effect if there is no
	 * shader called baseShaderName.
	 */
	static void AddShaderVariant(
		const std::string & shaderName,
		const std::string & baseShaderName,
		const std::string & define);

	static void AddShaderVariant(
		const std::string& shaderName,
		const std::string& baseShaderName,
		const std::vector<std::string>& defines,
		const std::map<std::string, std::string>& snippets = {});

	/**
	 * Get a shader from the pool by its name.
	 */
	static std::shared_ptr<ShaderProgram> GetShader(const std::string & shaderName);

	static void ReloadShaders();

	/**
	 * Load shaders from JSON file
	 */
	static bool Deserialize(const rapidjson::Value & json);

	/**
	 * Remove all shaders from pool (invalidates all weak pointers to it
	 */
	static void Clear();

private:
	ShaderPool();
	ShaderPool& operator=(const ShaderPool&) = default;
	ShaderPool(const ShaderPool&) = default;

	void addShader(
		const std::string& shaderName,
		const std::string& baseFile,
		ShaderProgram::ShaderProgramType type,
		const std::vector<std::string>& defines = {},
		const std::map<std::string, std::string>& snippets = {});

	void addShaderVariant(
		const std::string& shaderName,
		const std::string& baseShaderName,
		const std::vector<std::string>& defines,
		const std::map<std::string, std::string>& snippets = {});

	std::shared_ptr<ShaderProgram> getShader(const std::string & shaderName);

	void reloadShaders();
	bool deserialize(const rapidjson::Value & json);
	void clear();

private:
	struct ShaderInfo {
		std::string baseFile;
		ShaderProgram::ShaderProgramType type;
		std::vector<std::string> defines;
	};

private:
	static ShaderPool s_instance;
	std::map<std::string, std::shared_ptr<ShaderProgram>> m_shaders;
	std::map<std::string, ShaderInfo> m_defaultShaders;
};
