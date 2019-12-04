// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************=

#include "ShadowMap.h"
#include "Logger.h"

#include <glm/glm.hpp>

using namespace std;

ShadowMap::ShadowMap(const glm::vec3 & lightPosition, const glm::vec3 & lightLookAt, size_t size, const std::vector<ColorLayerInfo> & colorLayerInfos)
	: Framebuffer(size, size, colorLayerInfos)
{
	m_camera.setResolution(static_cast<int>(size), static_cast<int>(size));  // must be first
	setLookAt(lightPosition, lightLookAt);
}

void ShadowMap::setLookAt(const glm::vec3 & position, const glm::vec3 & lookAt) {
	m_camera.setViewMatrix(glm::lookAt(position, lookAt, glm::vec3(0, 0, 1)));
	//m_camera.setProjectionMatrix(glm::ortho<float>(-3.0, 3.0, -3.0, 3.0, 0.0, 3.0));
	m_camera.setProjectionMatrix(glm::perspective<float>(glm::radians(35.f), 1.f, 0.1f, 20.f));
}

void ShadowMap::setFov(float fov)
{
	m_camera.setFov(fov);
}
