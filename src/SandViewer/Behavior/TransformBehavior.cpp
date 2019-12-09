#include <vector>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#include "Logger.h"
#include "utils/jsonutils.h"
#include "TransformBehavior.h"
#include "ResourceManager.h"
#include "AnimationManager.h"

bool TransformBehavior::deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations)
{
	if (json.HasMember("modelMatrix")) {
		auto& mat = json["modelMatrix"];
		if (mat.IsArray()) {
			bool valid = true;
			for (int i = 0; i < 16 && valid; ++i) valid = valid && mat[i].IsNumber();
			if (!valid) {
				ERR_LOG << "modelMatrix must be either a 16-float array or an object";
			}
			else {
				float data[16];
				for (int i = 0; i < 16; ++i) data[i] = mat[i].GetFloat();
				m_modelMatrix = glm::make_mat4(data);
			}
		}
		else if (mat.IsObject()) {
			bool valid = mat.HasMember("buffer") && mat.HasMember("startFrame") && mat["buffer"].IsString() && mat["startFrame"].IsNumber();
			if (!valid) {
				ERR_LOG << "modelMatrix object must provide 'buffer' (string) and 'startFrame' (number) fields";
			}
			else if (animations) {
				std::string path = ResourceManager::resolveResourcePath(mat["buffer"].GetString());
				LOG << "Loading model animation from " << path << "...";

				std::ifstream file(path, std::ios::binary | std::ios::ate);
				if (!file.is_open()) {
					ERR_LOG << "Could not open modelMatrix buffer file: " << path;
				}
				std::streamsize size = file.tellg() / sizeof(float);
				file.seekg(0, std::ios::beg);
				std::shared_ptr<float[]> buffer(new float[size]);
				if (!file.read(reinterpret_cast<char*>(buffer.get()), size * sizeof(float))) {
					ERR_LOG << "Could not read modelMatrix buffer from file: " << path;
				}
				if (size % 16 != 0) {
					ERR_LOG << "modelMatrix buffer size must be a multiple of 16 (in file " << path << ")";
				}

				auto& postTransformJson = mat["postTransform"];
				if (postTransformJson.IsArray()) {
					bool valid = true;
					for (int i = 0; i < 16 && valid; ++i) valid = valid && postTransformJson[i].IsNumber();
					if (!valid) {
						ERR_LOG << "postTransform must be a 16-float array";
					} else {
						float data[16];
						for (int i = 0; i < 16; ++i) data[i] = postTransformJson[i].GetFloat();
						m_postTransform = glm::make_mat4(data);
					}
				}

				int startFrame = mat["startFrame"].GetInt();
				int endFrame = startFrame + static_cast<int>(size) / 16 - 1;
				int freezeAfterFrame = endFrame;

				if (mat["freezeAfterFrame"].IsInt()) {
					freezeAfterFrame = mat["freezeAfterFrame"].GetInt();
				}

				animations->addAnimation([startFrame, endFrame, freezeAfterFrame, buffer, this](float time, int frame) {
					int offset = (frame - startFrame) % (endFrame - startFrame + 1);
					offset = std::min(offset, freezeAfterFrame);
					m_modelMatrix = m_postTransform * glm::make_mat4(buffer.get() + 16 * offset);
				});
			}
		}
	}
	return true;
}
