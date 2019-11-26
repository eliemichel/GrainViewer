#pragma once

#include <glm/glm.hpp>
#include "Behavior.h"

class TransformBehavior : public Behavior {
public:
	const glm::mat4 & modelMatrix() const { return m_modelMatrix; }

public:
	bool deserialize(const rapidjson::Value & json) override;

private:
	glm::mat4 m_modelMatrix;
};

registerBehaviorType(TransformBehavior)
