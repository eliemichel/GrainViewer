// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <glad/glad.h>

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Framebuffer;

class Camera {
public:
	struct OutputSettings {
		bool autoOutputResolution = true;
		int width;
		int height;
		std::string outputFrameBase;
		bool isRecordEnabled = false;
	};

public:
	Camera();
	~Camera();
	virtual void update(float time);

	void initUbo();
	void updateUbo();
	GLuint ubo() const { return m_ubo; }

	inline glm::vec3 position() const { return m_position; }
	inline glm::mat4 viewMatrix() const { return m_viewMatrix; }
	inline glm::mat4 projectionMatrix() const { return m_projectionMatrix; }

	inline glm::vec2 resolution() const { return m_resolution; }
	void setResolution(glm::vec2 resolution);
	inline void setResolution(int w, int h) { setResolution(glm::vec2(static_cast<float>(w), static_cast<float>(h))); }
	void setFreezeResolution(bool freeze);
	void setFov(float fov);

	OutputSettings & outputSettings() { return m_outputSettings; }
	const OutputSettings & outputSettings() const { return m_outputSettings; }

	std::shared_ptr<Framebuffer> framebuffer() const { return m_framebuffer; }

	inline void setViewMatrix(glm::mat4 matrix) { m_viewMatrix = matrix; }  // use with caution
	inline void setProjectionMatrix(glm::mat4 matrix) { m_projectionMatrix = matrix; }

	inline void startMouseRotation() { m_isMouseRotationStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseRotation() { m_isMouseRotationStarted = false; }

	inline void startMouseZoom() { m_isMouseZoomStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseZoom() { m_isMouseZoomStarted = false; }
	
	inline void startMousePanning() { m_isMousePanningStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMousePanning() { m_isMousePanningStarted = false; }

	inline void tiltLeft() { tilt(-0.1f); }
	inline void tiltRight() { tilt(0.1f); }

	void updateMousePosition(float x, float y);

protected:
	/**
	* Called when mouse moves and rotation has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and zoom has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and panning has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMousePanning(float x1, float y1, float x2, float y2) {}

	virtual void tilt(float theta) {}

protected:
	glm::vec3 m_position;
	glm::vec2 m_resolution;
	glm::mat4 m_viewMatrix, m_projectionMatrix;
	float m_fov;

	bool m_isMouseRotationStarted, m_isMouseZoomStarted, m_isMousePanningStarted;
	bool m_isLastMouseUpToDate;
	float m_lastMouseX, m_lastMouseY;

	bool m_freezeResolution;

	std::shared_ptr<Framebuffer> m_framebuffer;

	OutputSettings m_outputSettings;

	GLuint m_ubo;
};
