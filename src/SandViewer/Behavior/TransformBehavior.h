#pragma once

#include <glm/glm.hpp>
#include "Behavior.h"

class TransformBehavior : public Behavior {
public:
	const glm::mat4 & modelMatrix() const { return m_modelMatrix; }

public:
	bool deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations) override;

private:
	glm::mat4 m_modelMatrix;
	glm::mat4 m_postTransform = glm::mat4(1);
};

registerBehaviorType(TransformBehavior)
