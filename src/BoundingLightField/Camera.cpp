// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <iostream>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Logger.h"
#include "Camera.h"
#include "Framebuffer.h"

Camera::Camera()
	: m_isMouseRotationStarted(false)
	, m_isMouseZoomStarted(false)
	, m_isMousePanningStarted(false)
	, m_isLastMouseUpToDate(false)
	, m_fov(45.0f)
	, m_orthographicScale(1.0f)
	, m_freezeResolution(false)
{
	initUbo();
	//updateUbo();  // included in setResolution
	setResolution(800, 600);
}

Camera::~Camera() {
	glDeleteBuffers(1, &m_ubo);
}

void Camera::update(float time) {
	m_position = glm::vec3(3.f * cos(time), 3.f * sin(time), 0.f);
	m_viewMatrix = glm::lookAt(
		m_position,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
}

void Camera::initUbo() {
	glCreateBuffers(1, &m_ubo);
	glNamedBufferStorage(m_ubo, static_cast<GLsizeiptr>((3 * 16 + 4) * sizeof(GLfloat)), NULL, GL_DYNAMIC_STORAGE_BIT);
}
void Camera::updateUbo() {
	glm::mat4 inverseViewMatrix = inverse(m_viewMatrix);
	GLsizeiptr matSize = static_cast<GLsizeiptr>(16 * sizeof(GLfloat));
	GLsizeiptr vec2Size = static_cast<GLsizeiptr>(2 * sizeof(GLfloat));
	GLsizeiptr matOffset = static_cast<GLintptr>(16 * sizeof(GLfloat));
	glNamedBufferSubData(m_ubo, 0, matSize, glm::value_ptr(m_viewMatrix));
	glNamedBufferSubData(m_ubo, matOffset, matSize, glm::value_ptr(m_projectionMatrix));
	glNamedBufferSubData(m_ubo, 2 * matOffset, matSize, glm::value_ptr(inverseViewMatrix));
	glNamedBufferSubData(m_ubo, 3 * matOffset, vec2Size, glm::value_ptr(resolution()));
}

void Camera::setResolution(glm::vec2 resolution)
{
	if (m_freezeResolution) {
		return;
	}

	m_resolution = resolution;
	switch (m_projectionType) {
	case PerspectiveProjection:
		if (m_resolution.x > 0.0f && m_resolution.y > 0.0f) {
			m_projectionMatrix = glm::perspectiveFov(glm::radians(m_fov), m_resolution.x, m_resolution.y, 0.001f, 10000.f);
		}
		break;
	case OrthographicProjection:
	{
		float ratio = m_resolution.x > 0.0f ? m_resolution.y / m_resolution.x : 1.0f;
		m_projectionMatrix = glm::ortho(-m_orthographicScale, m_orthographicScale, -m_orthographicScale * ratio, m_orthographicScale * ratio, 0.001f, 10000.f);
		break;
	}
	}
	updateUbo();
}

void Camera::setFreezeResolution(bool freeze)
{
	m_freezeResolution = freeze;
	if (m_freezeResolution) {
		size_t width = static_cast<size_t>(m_resolution.x);
		size_t height = static_cast<size_t>(m_resolution.y);
		const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
		m_framebuffer = std::make_shared<Framebuffer>(width, height, colorLayerInfos);
	} else {
		m_framebuffer = nullptr;
	}
}

void Camera::setFov(float fov)
{
	m_fov = fov;
	setResolution(m_resolution); // update projection matrix
}

void Camera::setOrthographicScale(float orthographicScale)
{
	m_orthographicScale = orthographicScale;
	setResolution(m_resolution); // update projection matrix
}

void Camera::setProjectionType(ProjectionType projectionType)
{
	m_projectionType = projectionType;
	setResolution(m_resolution); // update projection matrix
}

void Camera::updateMousePosition(float x, float y)
{
	if (m_isLastMouseUpToDate) {
		if (m_isMouseRotationStarted) {
			updateDeltaMouseRotation(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMouseZoomStarted) {
			updateDeltaMouseZoom(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMousePanningStarted) {
			updateDeltaMousePanning(m_lastMouseX, m_lastMouseY, x, y);
		}
	}
	m_lastMouseX = x;
	m_lastMouseY = y;
	m_isLastMouseUpToDate = true;
}
