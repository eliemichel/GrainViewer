#include <iostream>
#include <fstream>
#include <regex>
#include <filesystem>
namespace fs = std::filesystem;

#include <rapidjson/document.h>

#include "Logger.h"
#include "ResourceManager.h"
#include "RuntimeObject.h"
#include "utils/strutils.h"
#include "utils/fileutils.h"
#include "Scene.h"
#include "ShaderPool.h"

// TODO: Find a way to avoid this function
#include "Behavior/MeshDataBehavior.h"
#include "Behavior/MeshRenderer.h"
#include "Behavior/ImpostorCloudRenderer.h"
#include "Behavior/SandRenderer.h"
#include "Behavior/TransformBehavior.h"
static void addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type)
{
#define handleType(T) if (type == TypeName<T>().Get()) { b = IBehaviorHolder::addBehavior<T>(obj); }
	handleType(MeshDataBehavior);
	handleType(MeshRenderer);
	handleType(ImpostorCloudRenderer);
	handleType(SandRenderer);
	handleType(TransformBehavior);
#undef handleType
}

bool Scene::load(const std::string & filename)
{
	clear();
	m_filename = filename;

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

	if (root.HasMember("cameras")) {
		auto& cameras = root["cameras"];
		if (!cameras.IsArray()) { ERR_LOG << "cameras field must be an array."; return false; }

		m_viewportCameraIndex = 0;
		for (rapidjson::SizeType i = 0; i < cameras.Size(); i++) {
			auto& cameraJson = cameras[i];

			auto camera = std::make_shared<TurntableCamera>();

			if (cameraJson.HasMember("resolution")) {
				auto& resolutionJson = cameraJson["resolution"];
				if (resolutionJson.IsString()) {
					std::string resolution = resolutionJson.GetString();
					if (resolution != "auto") {
						ERR_LOG << "Invalid resolution '" << resolution << "'. Resolution must either be an array of two int elements or the string 'auto'";
					}
					else {
						camera->setFreezeResolution(false);
					}
				} else if (resolutionJson.IsArray()) {
					if (resolutionJson.Size() != 2 || !resolutionJson[0].IsInt() || !resolutionJson[1].IsInt()) {
						ERR_LOG << "Invalid resolution. Resolution must either be an array of two int elements or the string 'auto'";
					} else {
						int width = resolutionJson[0].GetInt();
						int height = resolutionJson[1].GetInt();
						camera->setResolution(width, height);
						camera->setFreezeResolution(true);
					}
				} else {
					ERR_LOG << "Invalid resolution '" << resolutionJson.GetString() << "'. Resolution must either be an array of two int elements or the string 'auto'";
				}
			}

			if (cameraJson.HasMember("outputResolution")) {
				auto& resolutionJson = cameraJson["outputResolution"];
				if (resolutionJson.IsArray()) {
					if (resolutionJson.IsString()) {
						std::string resolution = resolutionJson.GetString();
						if (resolution != "auto") {
							ERR_LOG << "Invalid output resolution '" << resolution << "'. Output resolution must either be an array of two int elements or the string 'auto'";
						}
						else {
							camera->outputSettings().autoOutputResolution = true;
						}
					}
					else if (resolutionJson.Size() != 2 || !resolutionJson[0].IsInt() || !resolutionJson[1].IsInt()) {
						ERR_LOG << "Invalid output resolution. Output resolution must either be an array of two int elements or the string 'auto'";
					}
					else {
						auto& s = camera->outputSettings();
						s.width = resolutionJson[0].GetInt();
						s.height = resolutionJson[1].GetInt();
						s.autoOutputResolution = false;
					}
				}
				else {
					ERR_LOG << "Invalid output resolution '" << resolutionJson.GetString() << "'. Output resolution must either be an array of two int elements or the string 'auto'";
				}
			}

			if (cameraJson.HasMember("isRecordEnabled") && cameraJson["isRecordEnabled"].IsBool()) {
				camera->outputSettings().isRecordEnabled = cameraJson["isRecordEnabled"].GetBool();
			}

			if (cameraJson.HasMember("outputFrameBase") && cameraJson["outputFrameBase"].IsString()) {
				std::string outputFrameBase = cameraJson["outputFrameBase"].GetString();
				outputFrameBase = std::regex_replace(
					outputFrameBase,
					std::regex("\\$BASEFILE"),
					fs::path(m_filename).stem().string()
				);
				outputFrameBase = ResourceManager::resolveResourcePath(outputFrameBase);
				camera->outputSettings().outputFrameBase = outputFrameBase;
			}

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
					b->deserialize(behaviorJson);
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

	reloadShaders();

	DEBUG_LOG << "Loading done.";

	return true;
}
