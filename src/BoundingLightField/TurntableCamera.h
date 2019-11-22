#pragma once

#include "Camera.h"

class TurntableCamera : public Camera {
public:
	TurntableCamera()
		: Camera()
		, m_zoom(3.f)
		, m_sensitivity(0.003f)
		, m_zoomSensitivity(0.01f)
	{
		m_quat = glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::quat(0.f, 0.f, 0.f, 1.f);
		updateViewMatrix();
	}

	void update(float time) override {}

protected:
	void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) override;
	void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) override;
	void updateDeltaMousePanning(float x1, float y1, float x2, float y2) override;
	void tilt(float theta);

private:
	/**
	* Construct view matrix given quat, center and zoom
	*/
	void updateViewMatrix();

private:
	glm::vec3 m_center;
	glm::quat m_quat;
	float m_zoom;
	float m_sensitivity;  // relative to screen size
	float m_zoomSensitivity;
};

