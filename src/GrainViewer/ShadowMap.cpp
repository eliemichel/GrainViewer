/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#include "ShadowMap.h"
#include "Logger.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

ShadowMap::ShadowMap(
	const glm::vec3 & lightPosition,
	const glm::vec3 & lightLookAt,
	size_t size,
	const std::vector<ColorLayerInfo> & colorLayerInfos
)
	: Framebuffer(size, size, colorLayerInfos)
{
	m_camera.setResolution(static_cast<int>(size), static_cast<int>(size));  // must be first
	setLookAt(lightPosition, lightLookAt);
}

void ShadowMap::setLookAt(const glm::vec3 & position, const glm::vec3 & lookAt) {
	m_camera.setViewMatrix(glm::lookAt(position, lookAt, glm::vec3(0, 0, 1)));
	updateProjectionMatrix();
}

void ShadowMap::setProjection(float fov, float nearDistance, float farDistance)
{
	m_fov = fov;
	m_near = nearDistance;
	m_far = farDistance;
	updateProjectionMatrix();
}

void ShadowMap::updateProjectionMatrix()
{
	m_camera.setProjectionMatrix(glm::perspective<float>(glm::radians(m_fov), 1.f, m_near, m_far));
	m_camera.updateUbo();
}
