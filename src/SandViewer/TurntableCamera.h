#pragma once

#include "Camera.h"

class TurntableCamera : public Camera {
public:
	TurntableCamera();

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

