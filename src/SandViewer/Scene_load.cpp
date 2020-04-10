#include <iostream>
#include <fstream>
#include <regex>

#include <rapidjson/document.h>

#include "Logger.h"
#include "ResourceManager.h"
#include "RuntimeObject.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"
#include "Scene.h"
#include "ShaderPool.h"
#include "EnvironmentVariables.h"
#include "BehaviorRegistry.h"
#include "Behavior.h"
#include "GlDeferredShader.h"

bool Scene::load(const std::string & filename)
{
	clear();
	m_filename = filename;

	EnvironmentVariables env;
	env.baseFile = fs::path(m_filename).stem().string();

	rapidjson::Document d;
	bool valid;
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

	valid = d.IsObject() && d.HasMember("augen");
	if (!valid) { ERR_LOG << "JSON scene must be an object with a field called 'augen'."; return false; }

	rapidjson::Value& root = d["augen"];

	if (root.HasMember("shaders")) {
		if (!ShaderPool::Deserialize(root["shaders"])) {
			return false;
		}
	}

	if (root.HasMember("deferredShader")) {
		if (!m_deferredShader->deserialize(root["deferredShader"])) {
			return false;
		}
	}

	if (!m_world->deserialize(root)) { // look at both root["world"] and root["lights"]
		return false;
	}
	m_world->start();

	if (root.HasMember("cameras")) {
		auto& cameras = root["cameras"];
		if (!cameras.IsArray()) { ERR_LOG << "cameras field must be an array."; return false; }

		m_viewportCameraIndex = 0;
		for (rapidjson::SizeType i = 0; i < cameras.Size(); i++) {
			auto& cameraJson = cameras[i];

			auto camera = std::make_shared<TurntableCamera>();
			camera->deserialize(cameraJson, env, m_animationManager);
			m_cameras.push_back(camera);
		}
	}
	else {
		m_cameras.push_back(std::make_shared<TurntableCamera>());
		m_viewportCameraIndex = 0;
	}

	if (root.HasMember("objects")) {
		auto& objects = root["objects"];
		if (!objects.IsArray()) { ERR_LOG << "objects field must be an array."; return false; }

		for (rapidjson::SizeType i = 0; i < objects.Size(); i++) {
			auto& o = objects[i];

			if (o.HasMember("ignore") && o["ignore"].IsBool() && o["ignore"].GetBool()) {
				continue;
			}

			auto obj = std::make_shared<RuntimeObject>();
			obj->deserialize(o);

			// Components
			std::vector<std::shared_ptr<Behavior>> behaviors;
			const auto& d = o["behaviors"];
			behaviors.reserve(static_cast<size_t>(d.Size()));
			for (rapidjson::SizeType i = 0; i < d.Size(); i++) {
				const rapidjson::Value& behaviorJson = d[i];
				if (!(behaviorJson.HasMember("type") && behaviorJson["type"].IsString())) {
					continue;
				}
				const std::string & type = behaviorJson["type"].GetString();
				std::shared_ptr<Behavior> b;
				BehaviorRegistry::addBehavior(b, obj, type);
				if (b) {
					b->deserialize(behaviorJson, env, m_animationManager);
					if (behaviorJson.HasMember("enabled") && behaviorJson["enabled"].IsBool()) {
						b->setEnabled(behaviorJson["enabled"].GetBool());
					}
					behaviors.push_back(b);
				}
				else {
					ERR_LOG << "Unknown behavior type: " << type;
				}
			}

			obj->start();
			m_objects.push_back(obj);
		}
	}

	if (root.HasMember("scene")) {
		auto& scene = root["scene"];
		if (scene.HasMember("quitAfterFrame")) {
			if (scene["quitAfterFrame"].IsInt()) {
				m_quitAfterFrame = scene["quitAfterFrame"].GetInt();
			} else {
				WARN_LOG << "'quitAfterFrame' field of 'scene' must be an integer";
			}
		}

		if (scene.HasMember("outputStats")) {
			if (scene["outputStats"].IsString()) {
				m_outputStats = scene["outputStats"].GetString();
				m_outputStats = std::regex_replace(m_outputStats, std::regex("\\$BASEFILE"), env.baseFile);
				m_outputStats = ResourceManager::resolveResourcePath(m_outputStats);
			}
			else {
				WARN_LOG << "'outputStats' field of 'scene' must be a string";
			}
		}

		if (scene.HasMember("statsCountColors")) {
			if (scene["statsCountColors"].IsArray()) {
				const auto & a = scene["statsCountColors"].GetArray();
				m_statsCountColors.resize(a.Size());
				for (rapidjson::SizeType i = 0; i < a.Size(); ++i) {
					if (!a[i].IsArray() || a[i].GetArray().Size() != 3) {
						WARN_LOG << "'statsCountColors' field of 'scene' must be an array of vec3";
						continue;
					}
					const auto & v = a[i].GetArray();
					m_statsCountColors[i].x = v[0].GetFloat();
					m_statsCountColors[i].y = v[1].GetFloat();
					m_statsCountColors[i].z = v[2].GetFloat();
				}
			}
			else {
				WARN_LOG << "'statsCountColors' field of 'scene' must be an array of vec3";
			}
		}
	}

	// Add an extra camera to visualize occlusion
	auto camera = std::make_shared<TurntableCamera>();
	camera->properties().displayInViewport = false;
	camera->properties().controlInViewport = false;
	m_cameras.push_back(camera);
	m_occlusionCameraIndex = static_cast<int>(m_cameras.size() - 1);

	// Scene start

	const glm::vec2 & res = viewportCamera()->resolution();

	reloadShaders();

	// Start color output stats
	if (!m_outputStats.empty()) {
		fs::create_directories(fs::path(m_outputStats).parent_path());
		m_outputStatsFile.open(m_outputStats);
		m_outputStatsFile << "frame";
		for (const auto &c : m_statsCountColors) {
			m_outputStatsFile << ";<" << c.x << "," << c.y << "," << c.z << ">";
		}
		m_outputStatsFile << "\n";
	}

	DEBUG_LOG << "Loading done.";

	return true;
}
