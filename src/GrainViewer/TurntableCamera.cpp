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

#include <algorithm>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>

#include "utils/jsonutils.h"
#include "TurntableCamera.h"

using glm::vec3;
using glm::quat;

TurntableCamera::TurntableCamera()
	: Camera()
	, m_center(0, 0, 0)
	, m_zoom(3.f)
	, m_sensitivity(0.003f)
	, m_zoomSensitivity(0.01f)
{
	m_quat = glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::quat(0.f, 0.f, 0.f, 1.f);
	updateViewMatrix();
}

void TurntableCamera::updateViewMatrix() {
	m_uniforms.viewMatrix = glm::translate(vec3(0.f, 0.f, -m_zoom)) * glm::toMat4(m_quat) * glm::translate(m_center);
	m_position = vec3(glm::inverse(m_uniforms.viewMatrix)[3]);

	updateUbo();
}

void TurntableCamera::updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float theta;

	// rotate around camera X axis by dy
	theta = dy * m_sensitivity;
	vec3 xAxis = vec3(glm::row(m_uniforms.viewMatrix, 0));
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * xAxis);

	// rotate around world Z axis by dx
	theta = dx * m_sensitivity;
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * vec3(0.f, 0.f, 1.f));

	updateViewMatrix();
}

void TurntableCamera::updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {
	float dy = y2 - y1;
	float fac = 1.f + dy * m_zoomSensitivity;
	m_zoom *= fac;
	m_zoom = std::max(m_zoom, 0.0001f);
	updateViewMatrix();

	float s = orthographicScale();
	s *= fac;
	s = std::max(s, 0.0001f);
	setOrthographicScale(s);
}

void TurntableCamera::updateDeltaMousePanning(float x1, float y1, float x2, float y2) {
	float dx = (x2 - x1) / 600.f * m_zoom;
	float dy = -(y2 - y1) / 300.f * m_zoom;

	// move center along the camera screen plane
	vec3 xAxis = vec3(glm::row(m_uniforms.viewMatrix, 0));
	vec3 yAxis = vec3(glm::row(m_uniforms.viewMatrix, 1));
	m_center += dx * xAxis + dy * yAxis;

	updateViewMatrix();
}

void TurntableCamera::tilt(float theta) {
	vec3 zAxis = vec3(glm::row(m_uniforms.viewMatrix, 2));
	m_quat *= quat(static_cast<float>(cos(theta / 2.f)), static_cast<float>(sin(theta / 2.f)) * zAxis);
	updateViewMatrix();
}

///////////////////////////////////////////////////////////////////////////////
// Serialization
///////////////////////////////////////////////////////////////////////////////

void TurntableCamera::deserialize(const rapidjson::Value& json, const EnvironmentVariables& env, std::shared_ptr<AnimationManager> animations)
{
	Camera::deserialize(json, env, animations);

	if (json.HasMember("turntable") && json["turntable"].IsObject()) {
		auto& turntableJson = json["turntable"];

		jrOption(turntableJson, "center", m_center, m_center);
		jrOption(turntableJson, "quat", m_quat, m_quat);
		jrOption(turntableJson, "zoom", m_zoom, m_zoom);
		jrOption(turntableJson, "sensitivity", m_sensitivity, m_sensitivity);
		jrOption(turntableJson, "zoomSensitivity", m_zoomSensitivity, m_zoomSensitivity);

		updateViewMatrix();
	}
}

std::ostream& TurntableCamera::serialize(std::ostream& out)
{
	out
		<< "\"turntable\": {\n"
		<< "  \"center\": [" << m_center[0] << ", " << m_center[1] << ", " << m_center[2] << "],\n"
		<< "  \"quat\": [" << m_quat[0] << ", " << m_quat[1] << ", " << m_quat[2] << ", " << m_quat[3] << "],\n"
		<< "  \"zoom\": " << m_zoom << ",\n"
		<< "  \"sensitivity\": " << m_sensitivity << ",\n"
		<< "  \"zoomSensitivity\": " << m_zoomSensitivity << "\n"
		<< "}"
		;
	return out;
}


