#pragma once

#include <glm/glm.hpp>
#include "Behavior.h"

class TransformBehavior : public Behavior {
public:
	const glm::mat4 & modelMatrix() const { return m_modelMatrix; }

	const glm::mat4 & preTransform() const { return m_preTransform; }
	void setPreTransform(const glm::mat4 & transform) { m_preTransform = transform; updateModelMatrix(); }

	const glm::mat4 & postTransform() const { return m_postTransform; }
	void setPostTransform(const glm::mat4 & transform) { m_postTransform = transform; updateModelMatrix(); }

public:
	bool deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations) override;

private:
	void updateModelMatrix();

private:
	glm::mat4 m_modelMatrix; // composited matrix
	glm::mat4 m_postTransform = glm::mat4(1);
	glm::mat4 m_transform;
	glm::mat4 m_preTransform = glm::mat4(1);
};
