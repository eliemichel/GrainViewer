#include <iostream>
#include <fstream>

#include <rapidjson/document.h>

#include "Logger.h"
#include "ResourceManager.h"
#include "RuntimeObject.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"
#include "Scene.h"
#include "ShaderPool.h"
#include "EnvironmentVariables.h"

// TODO: Find a way to avoid this function
#include "Behavior/MeshDataBehavior.h"
#include "Behavior/MeshRenderer.h"
#include "Behavior/ImpostorCloudRenderer.h"
#include "Behavior/SandRenderer.h"
#include "Behavior/TransformBehavior.h"
#include "Behavior/TestPrefixSumRenderer.h"
#include "Behavior/LightGizmo.h"
#include "Behavior/PointCloudDataBehavior.h"
#include "Behavior/FarSandRenderer.h"
static void addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type)
{
#define handleType(T) if (type == TypeName<T>().Get()) { b = IBehaviorHolder::addBehavior<T>(obj); }
	handleType(MeshDataBehavior);
	handleType(MeshRenderer);
	handleType(ImpostorCloudRenderer);
	handleType(SandRenderer);
	handleType(TransformBehavior);
	handleType(TestPrefixSumRenderer);
	handleType(LightGizmo);
	handleType(PointCloudDataBehavior);
	handleType(FarSandRenderer);
#undef handleType
}

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
		if (!m_deferredShader.deserialize(root["deferredShader"])) {
			return false;
		}
	}

	if (!m_world.deserialize(root)) { // look at both root["world"] and root["lights"]
		return false;
	}
	m_world.start();

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

			auto obj = std::make_shared<RuntimeObject>();

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
				addBehavior(b, obj, type);
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
	}

	reloadShaders();

	DEBUG_LOG << "Loading done.";

	return true;
}
