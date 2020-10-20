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

#include "Light.h"
#include "ShadowMap.h"

Light::Light(const glm::vec3 & position, const glm::vec3 & color, size_t shadowMapSize, bool isRich, bool hasShadowMap)
	: m_lightPosition(position)
	, m_lookAt(0.0, 0.0, 0.0)
	, m_lightColor(color)
	, m_isRich(isRich)
	, m_hasShadowMap(hasShadowMap)
{
	const std::vector<ColorLayerInfo> & colorLayerInfos = { { GL_RGBA32UI, GL_COLOR_ATTACHMENT0 } }; // debug
	m_shadowMap = std::make_unique<ShadowMap>(position, glm::vec3(0.f), shadowMapSize, colorLayerInfos);
}

void Light::setPosition(const glm::vec3 & position) {
	m_lightPosition = position;
	m_shadowMap->setLookAt(position, m_lookAt);
}

TurningLight::TurningLight(const glm::vec3 & position, const glm::vec3 & color, size_t shadowMapSize, bool isRich, bool hasShadowMap)
	: Light(position, color, shadowMapSize, isRich, hasShadowMap)
	, m_initialPosition(position)
{}


void TurningLight::update(float time) {
	const glm::vec3 & p = m_initialPosition;
	const float dtheta = 0.3f;
	float theta = time * dtheta;
	setPosition(glm::vec3(p.x * cos(theta) + p.y * sin(theta), p.y * cos(theta) - p.x * sin(theta), p.z));
}