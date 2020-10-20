// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

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
	//setPosition(glm::vec3(p.x, p.y * cos(theta) + p.z * sin(theta), p.z * cos(theta) - p.y * sin(theta)));
}