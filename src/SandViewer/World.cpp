#include "utils/jsonutils.h"

#include "ResourceManager.h"
#include "World.h"
#include "Light.h"
#include "Logger.h"

World::World()
{}

bool World::deserialize(const rapidjson::Value & json)
{
	bool valid;

	// World globals
	if (json.HasMember("world")) {
		auto& world = json["world"];

		valid = world.HasMember("type") && world["type"].IsString();
		if (!valid) { ERR_LOG << "world requires a string field 'type'"; return false; }
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

	// Lights
	if (json.HasMember("lights")) {
		const rapidjson::Value& lights = json["lights"];
		if (!lights.IsArray()) { ERR_LOG << "lights field must be an array."; return false; }

		for (rapidjson::SizeType i = 0; i < lights.Size(); i++) {
			const rapidjson::Value& l = lights[i];

			if (!l.HasMember("position")) { ERR_LOG << "light requires a position"; continue; }
			const rapidjson::Value& p = l["position"];
			valid = p.IsArray() && p.Size() == 3 && p[0].IsNumber() && p[1].IsNumber() && p[2].IsNumber();
			if (!valid) { ERR_LOG << "light position must be an array of 3 numbers"; continue; }
			glm::vec3 pos = glm::vec3(p[0].GetFloat(), p[1].GetFloat(), p[2].GetFloat());

			if (!l.HasMember("color")) { ERR_LOG << "light requires a color"; continue; }
			const rapidjson::Value& c = l["color"];
			valid = c.IsArray() && c.Size() == 3 && c[0].IsNumber() && c[1].IsNumber() && c[2].IsNumber();
			if (!valid) { ERR_LOG << "light color must be an array of 3 numbers"; continue; }
			glm::vec3 col = glm::vec3(c[0].GetFloat(), c[1].GetFloat(), c[2].GetFloat());

			bool isShadowMapRich;
			jrOption(l, "isShadowMapRich", isShadowMapRich, false);

			int shadowMapSize;
			jrOption(l, "shadowMapSize", shadowMapSize, 2048);

			// Add light
			auto light = std::make_shared<Light>(pos, col, shadowMapSize, isShadowMapRich);
			m_lights.push_back(light);
		}
	}

	return true;
}

void World::reloadShaders()
{}

void World::render(const Camera & camera) const
{}

void World::clear()
{
	m_lights.clear();
}
