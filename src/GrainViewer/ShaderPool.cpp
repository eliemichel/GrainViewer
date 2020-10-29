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

#include "Logger.h"
#include "ShaderPool.h"

#include <sstream>

#undef GetObject

ShaderPool ShaderPool::s_instance = ShaderPool();

///////////////////////////////////////////////////////////////////////////////
// Public static methods
///////////////////////////////////////////////////////////////////////////////

void ShaderPool::AddShader(
	const std::string & shaderName,
	const std::string & baseFile,
	ShaderProgram::ShaderProgramType type,
	const std::vector<std::string> & defines,
	const std::map<std::string, std::string> & snippets)
{
	s_instance.addShader(shaderName, baseFile, type, defines, snippets);
}

void ShaderPool::AddShaderVariant(
	const std::string & shaderName,
	const std::string & baseShaderName,
	const std::string & define)
{
	std::vector<std::string> d = { define };
	s_instance.addShaderVariant(shaderName, baseShaderName, d);
}

void ShaderPool::AddShaderVariant(
	const std::string & shaderName,
	const std::string & baseShaderName,
	const std::vector<std::string> & defines,
	const std::map<std::string, std::string> & snippets)
{
	s_instance.addShaderVariant(shaderName, baseShaderName, defines, snippets);
}

std::shared_ptr<ShaderProgram> ShaderPool::GetShader(const std::string & shaderName)
{
	return s_instance.getShader(shaderName);
}

void ShaderPool::ReloadShaders()
{
	s_instance.reloadShaders();
}

bool ShaderPool::Deserialize(const rapidjson::Value & json)
{
	return s_instance.deserialize(json);
}

void ShaderPool::Clear() {
	s_instance.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Private singleton methods
///////////////////////////////////////////////////////////////////////////////

ShaderPool::ShaderPool()
{
	m_defaultShaders.insert({
		"PrefixSum",
		{ "prefix-sum", ShaderProgram::ComputeShader, {} }
	});
	m_defaultShaders.insert({
		"LightGizmo",
		{ "light-gizmo", ShaderProgram::RenderShader,{} }
	});
	m_defaultShaders.insert({
		"GenerateMipmapDepthBuffer",
		{ "generate-mipmap-zbuffer", ShaderProgram::RenderShader, {} }
	});
	m_defaultShaders.insert({
		"DepthToColorBuffer",
		{ "depth-to-color", ShaderProgram::RenderShader, {} }
	});
	m_defaultShaders.insert({
		"GltfMesh",
		{ "gltf-mesh", ShaderProgram::RenderShader, {} }
	});
	m_defaultShaders.insert({
		"MipMapUsingAlpha",
		{ "mipmap-using-alpha", ShaderProgram::RenderShader, {} }
	});
	m_defaultShaders.insert({
		"BakeImpostorAtlas",
		{ "bake-impostor-atlas", ShaderProgram::RenderShader, {} }
	});
	m_defaultShaders.insert({
		"BakeImpostorAtlas_Blit",
		{ "bake-impostor-atlas", ShaderProgram::RenderShader, { "BLIT" } }
	});
}

void ShaderPool::addShader(
	const std::string & shaderName,
	const std::string & baseFile,
	ShaderProgram::ShaderProgramType type,
	const std::vector<std::string> & defines,
	const std::map<std::string, std::string>& snippets)
{
	if (m_shaders.count(shaderName) > 0) {
		WARN_LOG << "Duplicate shader name: " << shaderName << " (ignoring duplicates)";
		return;
	}
	m_shaders[shaderName] = std::make_shared<ShaderProgram>(baseFile);
	m_shaders[shaderName]->setType(type);
	for (const std::string& def : defines) {
		m_shaders[shaderName]->define(def);
	}
	for (const auto& s : snippets) {
		m_shaders[shaderName]->setSnippet(s.first, s.second);
	}
	m_shaders[shaderName]->load();
}

void ShaderPool::addShaderVariant(
	const std::string & shaderName,
	const std::string & baseShaderName,
	const std::vector<std::string> & defines,
	const std::map<std::string, std::string>& snippets)
{
	auto baseShader = getShader(baseShaderName);
	if (!baseShader) {
		WARN_LOG << "Cannot add variant to unexistant shader: " << baseShaderName;
		return;
	}

	bool alreadyDefined = true;
	for (const auto & def : defines) {
		if (baseShader->getDefines().count(def) == 0) {
			alreadyDefined = false;
			break;
		}
	}

	if (alreadyDefined) {
		m_shaders[shaderName] = baseShader;
	} else {
		m_shaders[shaderName] = std::make_shared<ShaderProgram>(baseShader->shaderName());
		m_shaders[shaderName]->copy(*baseShader);
		for (const auto & def : defines) {
			if (baseShader->getDefines().count(def) == 0) {
				m_shaders[shaderName]->define(def);
			}
		}
		m_shaders[shaderName]->load();
	}

	// (override snippets even if already defined)
	for (const auto& s : snippets) {
		m_shaders[shaderName]->setSnippet(s.first, s.second);
	}
}

std::shared_ptr<ShaderProgram> ShaderPool::getShader(const std::string & shaderName)
{
	if (m_shaders.count(shaderName) > 0) {
		return m_shaders.at(shaderName);
	} else if (m_defaultShaders.count(shaderName) > 0) {
		auto& info = m_defaultShaders[shaderName];
		addShader(shaderName, info.baseFile, info.type, info.defines);
		return m_shaders.at(shaderName);
	} else {
		return nullptr;
	}
}

void ShaderPool::reloadShaders()
{
	LOG << "Reloading shaders...";
	for (auto const& e : m_shaders)
	{
		e.second->load();
	}
}

bool ShaderPool::deserialize(const rapidjson::Value & json)
{
	LOG << "Loading shaders...";
	if (!json.IsObject()) { ERR_LOG << "shaders field must be an object."; return false; }

	for (auto it = json.MemberBegin(), end = json.MemberEnd(); it != end; ++it) {
		std::string name = it->name.GetString();
		std::vector<std::string> defines;
		std::map<std::string,std::string> snippets;
		std::string baseFile;
		ShaderProgram::ShaderProgramType type = ShaderProgram::RenderShader;
		std::ostringstream definesDebug;
		if (it->value.IsString()) {
			baseFile = it->value.GetString();
		}
		else if (it->value.IsObject()) {
			if (it->value.HasMember("baseFile") && it->value["baseFile"].IsString()) {
				baseFile = it->value["baseFile"].GetString();
			}
			if (it->value.HasMember("defines") && it->value["defines"].IsArray()) {
				for (rapidjson::SizeType i = 0; i < it->value["defines"].Size(); i++) {
					auto& def = it->value["defines"][i];
					if (!def.IsString()) {
						ERR_LOG << "Shader defines must be strings (in shader '" << name << "')";
						continue;
					}
					defines.push_back(def.GetString());
					if (i > 0) definesDebug << ", ";
					definesDebug << def.GetString();
				}
			}
			if (it->value.HasMember("snippets") && it->value["snippets"].IsObject()) {
				const auto& snippetsJson = it->value["snippets"];
				for (auto sit = snippetsJson.MemberBegin(), send = snippetsJson.MemberEnd(); sit != send; ++sit) {
					std::string name = sit->name.GetString();
					snippets[name] = sit->value.GetString();
				}
			}
			if (it->value.HasMember("type") && it->value["type"].IsString()) {
				std::string strtype = it->value["type"].GetString();
				if (strtype == "compute") {
					type = ShaderProgram::ComputeShader;
				} else if (strtype == "render") {
					type = ShaderProgram::RenderShader;
				}
				else {
					ERR_LOG << "Shader type not recognized: " << strtype << " (in shader '" << name << "')";
					continue;
				}
			}
		}
		else {
			ERR_LOG << "Shader entry must be either a string ofr an object. Ignoring shader '" << name << "'";
		}

		std::ostringstream snippetsDebug;
		for (const auto & s : snippets) {
			if (!snippetsDebug.str().empty()) snippetsDebug << ", ";
			snippetsDebug << "\"" << s.first << "\": \"" << s.second << "\"";
		}
		LOG << " - shader '" << name << "' loaded with defines [" << definesDebug.str() << "] and snippets {" << snippetsDebug.str() << "}";

		addShader(name, baseFile, type, defines, snippets);
	}
	return true;
}

void ShaderPool::clear()
{
	m_shaders.clear();
}
