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

// TODO: Find a way to avoid this function
#include "Behavior/MeshDataBehavior.h"
#include "Behavior/MeshRenderer.h"
static void addBehavior(std::shared_ptr<Behavior> & b, std::shared_ptr<RuntimeObject> & obj, const std::string & type)
{
	if (type == TypeName<MeshDataBehavior>().Get()) {
		b = IBehaviorHolder::addBehavior<MeshDataBehavior>(obj);
	}
	else if (type == TypeName<MeshRenderer>().Get()) {
		b = IBehaviorHolder::addBehavior<MeshRenderer>(obj);
	}
}

bool Scene::load(const std::string & filename)
{
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
		auto& json = root["shaders"];
		if (!json.IsObject()) { ERR_LOG << "shaders field must be an object."; return false; }

		for (auto it = json.MemberBegin(), end = json.MemberEnd(); it != end; ++it) {
			std::string name = it->name.GetString();
			std::vector<std::string> defines;
			std::string baseFile;
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
					}
				}
			}
			else {
				ERR_LOG << "Shader entry must be either a string ofr an object. Ignoring shader '" << name << "'";
			}
			ShaderPool::AddShader(name, baseFile, defines);
		}
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
