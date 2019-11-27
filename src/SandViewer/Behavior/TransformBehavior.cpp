#include <vector>

#include <glm/gtc/type_ptr.hpp>

#include "Logger.h"
#include "utils/jsonutils.h"
#include "TransformBehavior.h"

bool TransformBehavior::deserialize(const rapidjson::Value & json)
{
	std::vector<float> modelMatrixData;
	if (!jrArray<float>(json, "modelMatrix", modelMatrixData)) {
		ERR_LOG << "modelMatrix must be provided in TransformBehavior";
		return false;
	}
	if (modelMatrixData.size() != 16) {
		ERR_LOG << "modelMatrix must contain 16 floats";
		return false;
	}
	m_modelMatrix = glm::make_mat4(modelMatrixData.data());
	return true;
}
