#include <algorithm>

#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>

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
	m_viewMatrix = glm::translate(vec3(0.f, 0.f, -m_zoom)) * glm::toMat4(m_quat) * glm::translate(m_center);
	m_position = vec3(glm::inverse(m_viewMatrix)[3]);

	updateUbo();
}

void TurntableCamera::updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {
	float dx = x2 - x1;
	float dy = y2 - y1;
	float theta;

	// rotate around camera X axis by dy
	theta = dy * m_sensitivity;
	vec3 xAxis = vec3(glm::row(m_viewMatrix, 0));
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * xAxis);

	// rotate around world Z axis by dx
	theta = dx * m_sensitivity;
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * vec3(0.f, 0.f, 1.f));

	updateViewMatrix();
}

void TurntableCamera::updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {
	float dy = y2 - y1;
	m_zoom *= (1.f + dy * m_zoomSensitivity);
	m_zoom = std::max(m_zoom, 0.0001f);
	updateViewMatrix();
}

void TurntableCamera::updateDeltaMousePanning(float x1, float y1, float x2, float y2) {
	float dx = (x2 - x1) / 600.f * m_zoom;
	float dy = -(y2 - y1) / 300.f * m_zoom;

	// move center along the camera screen plane
	vec3 xAxis = vec3(glm::row(m_viewMatrix, 0));
	vec3 yAxis = vec3(glm::row(m_viewMatrix, 1));
	m_center += dx * xAxis + dy * yAxis;

	updateViewMatrix();
}

void TurntableCamera::tilt(float theta) {
	vec3 zAxis = vec3(glm::row(m_viewMatrix, 2));
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * zAxis);
	updateViewMatrix();
}

