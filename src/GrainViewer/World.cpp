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

#include "utils/jsonutils.h"

#include "ShaderProgram.h"
#include "ResourceManager.h"
#include "World.h"
#include "Light.h"
#include "Logger.h"
#include "ShaderPool.h"
#include "ShadowMap.h"
#include "RuntimeObject.h"
#include "RenderType.h"
#include "AnimationManager.h"

#include <glm/gtc/type_ptr.hpp>
#include <fstream>

World::World()
{}

bool World::deserialize(const rapidjson::Value & json, std::shared_ptr<AnimationManager> animations)
{
	bool valid;

	// World globals
	if (json.HasMember("world")) {
		auto& world = json["world"];
		jrOption(world, "shader", m_shaderName, m_shaderName);

		valid = world.HasMember("type") && world["type"].IsString();
		if (valid) {
			std::string t = world["type"].GetString();
			if (t == "cubemap" || t == "skybox") {  // "skybox" is for backward compat
				valid = world.HasMember("dirname") && world["dirname"].IsString();
				if (!valid) { ERR_LOG << "cubemap world requires a string field 'dirname'"; return false; }

				bool generateMipMaps, prefilterEnv;
				jrOption(world, "generateMipMaps", generateMipMaps, false);
				jrOption(world, "prefilterEnv", prefilterEnv, false);

				// Set skybox
				//std::string path = ResourceManager::resolveResourcePath(world["dirname"].GetString());
				//m_skybox.loadCubemap(path, generateMipMaps, prefilterEnv);
				//m_skybox.reloadShaders();
			}
			else if (t == "envmap") {
				valid = world.HasMember("envmap") && world["envmap"].IsString();
				if (!valid) { ERR_LOG << "envmap world requires a string field 'envmap'"; return false; }

				bool generateMipMaps, prefilterEnv;
				jrOption(world, "generateMipMaps", generateMipMaps, false);
				jrOption(world, "prefilterEnv", prefilterEnv, false);

				// Set skybox
				//m_skybox.loadEnvmap(world["envmap"].GetString(), generateMipMaps, prefilterEnv);
				//m_skybox.reloadShaders();
			}
			else {
				ERR_LOG << "unknown world type: " << t;
				return false;
			}
		}
	}

	// Lights
	if (json.HasMember("lights")) {
		const rapidjson::Value& lights = json["lights"];
		if (!lights.IsArray()) { ERR_LOG << "lights field must be an array."; return false; }

		for (rapidjson::SizeType i = 0; i < lights.Size(); i++) {
			const rapidjson::Value& l = lights[i];

			if (!l.HasMember("position")) { ERR_LOG << "light requires a position"; continue; }
			const rapidjson::Value& p = l["position"];
			glm::vec3 pos = glm::vec3(0.0);
			bool usePositionBuffer = p.IsObject() && p.HasMember("buffer") && animations;
			if (usePositionBuffer) {
				// Will load later, once light object is allocated
			} else {
				valid = p.IsArray() && p.Size() == 3 && p[0].IsNumber() && p[1].IsNumber() && p[2].IsNumber();
				if (!valid) { ERR_LOG << "light position must be an array of 3 numbers, or an object with a 'buffer' field"; continue; }
				pos = glm::vec3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat());
			}

			if (!l.HasMember("color")) { ERR_LOG << "light requires a color"; continue; }
			const rapidjson::Value& c = l["color"];
			valid = c.IsArray() && c.Size() == 3 && c[0].IsNumber() && c[1].IsNumber() && c[2].IsNumber();
			if (!valid) { ERR_LOG << "light color must be an array of 3 numbers"; continue; }
			glm::vec3 col = glm::vec3(c[0].GetFloat(), c[1].GetFloat(), c[2].GetFloat());

			bool isShadowMapRich;
			jrOption(l, "isShadowMapRich", isShadowMapRich, false);
			bool hasShadowMap;
			jrOption(l, "hasShadowMap", hasShadowMap, true);

			int shadowMapSize;
			jrOption(l, "shadowMapSize", shadowMapSize, 1024);
			float shadowMapFov;
			jrOption(l, "shadowMapFov", shadowMapFov, 35.0f);
			float shadowMapNear;
			jrOption(l, "shadowMapNear", shadowMapNear, 0.1f);
			float shadowMapFar;
			jrOption(l, "shadowMapFar", shadowMapFar, 20.0f);

			bool isTurning;
			jrOption(l, "isTurning", isTurning, false);

			// Add light
			auto light =
				isTurning
				? std::make_shared<TurningLight>(pos, col, shadowMapSize, isShadowMapRich, hasShadowMap)
				: std::make_shared<Light>(pos, col, shadowMapSize, isShadowMapRich, hasShadowMap);
			light->shadowMap().setProjection(shadowMapFov, shadowMapNear, shadowMapFar);

			if (usePositionBuffer) {
				std::string path = ResourceManager::resolveResourcePath(p["buffer"].GetString());
				LOG << "Loading light movement from " << path << "...";

				std::ifstream file(path, std::ios::binary | std::ios::ate);
				if (!file.is_open()) {
					ERR_LOG << "Could not open position buffer file: " << path;
					return false;
				}
				std::streamsize size = file.tellg() / sizeof(float);
				if (size % 3 != 0) {
					ERR_LOG << "position buffer size must be a multiple of 3 (in file " << path << ")";
					return false;
				}
				file.seekg(0, std::ios::beg);
				std::shared_ptr<float[]> buffer(new float[size]);
				if (!file.read(reinterpret_cast<char*>(buffer.get()), size * sizeof(float))) {
					ERR_LOG << "Could not read position buffer from file: " << path;
					return false;
				}

				int startFrame = p.HasMember("startFrame") ? p["startFrame"].GetInt() : 0;
				int frameCount = static_cast<int>(size) / 3 - 1;
				animations->addAnimation([startFrame, frameCount, buffer, light](float time, int frame) {
					glm::vec3 position = glm::make_vec3(buffer.get() + 3 * (frame - startFrame) % frameCount);
					light->setPosition(position);
				});
			}

			m_lights.push_back(light);
		}
	}

	return true;
}

void World::start()
{
	initVao();
	m_shader = ShaderPool::GetShader(m_shaderName);
}

void World::update(float time)
{
	for (auto light : lights()) {
		light->update(time);
	}
}

void World::reloadShaders()
{}

void World::onPreRender(const Camera& camera) const
{}

void World::render(const Camera & camera) const
{
	if (!m_shader || !m_shader->isValid()) {
		return;
	}

	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);

	m_shader->use();
	m_shader->bindUniformBlock("Camera", camera.ubo());

	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}

void World::renderShadowMaps(const std::vector<std::shared_ptr<RuntimeObject>> & objects) const
{
	if (!isShadowMapEnabled()) {
		return;
	}
	for (const auto& light : m_lights) {
		if (!light->hasShadowMap()) {
			continue;
		}

		light->shadowMap().bind();
		const glm::vec2 & sres = light->shadowMap().camera().resolution();
		glViewport(0, 0, static_cast<GLsizei>(sres.x), static_cast<GLsizei>(sres.y));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		const Camera& lightCamera = light->shadowMap().camera();

		// Pre-rendering
		for (const auto& obj : objects) {
			obj->onPreRender(lightCamera, *this, RenderType::ShadowMap);
		}

		// Main shadow map rendering
		for (const auto& obj : objects) {
			obj->render(lightCamera, *this, RenderType::ShadowMap);
		}
	}

}

void World::clear()
{
	m_lights.clear();
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
///////////////////////////////////////////////////////////////////////////////

void World::initVao() {
	GLfloat attributes[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};
	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	glCreateBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glNamedBufferData(m_vbo, sizeof(attributes), attributes, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexArrayAttrib(m_vao, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

