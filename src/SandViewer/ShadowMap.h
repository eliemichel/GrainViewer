// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
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
	void setFov(float fov);

	const Camera & camera() const { return m_camera; }

private:
	Camera m_camera;
};

