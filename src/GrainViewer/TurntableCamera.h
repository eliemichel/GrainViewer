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

#pragma once

#include "Camera.h"

#include <glm/gtc/quaternion.hpp>

class TurntableCamera : public Camera {
public:
	TurntableCamera();

	void update(float time) override {}

	void deserialize(const rapidjson::Value& json, const EnvironmentVariables& env, std::shared_ptr<AnimationManager> animations) override;
	std::ostream& serialize(std::ostream& out) override;

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

