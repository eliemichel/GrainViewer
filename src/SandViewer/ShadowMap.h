// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "Framebuffer.h"
#include "Camera.h"

class ShadowMap : public Framebuffer {
public:
	ShadowMap(const glm::vec3 & lightPosition, const glm::vec3 & lightLookAt = glm::vec3(0.0), size_t size = 1024, const std::vector<ColorLayerInfo> & colorLayerInfos = {});

	void setLookAt(const glm::vec3 & position, const glm::vec3 & lookAt);
	void setProjection(float fov, float near, float far);

	const Camera & camera() const { return m_camera; }

private:
	void updateProjectionMatrix();

private:
	Camera m_camera;
	float m_fov = 35.0f;
	float m_near = 0.1f;
	float m_far = 20.f;
};

