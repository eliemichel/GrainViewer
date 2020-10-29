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

#include "Behavior.h"

#include <glm/glm.hpp>

/**
 * Define the global affine transform to apply to the object at draw time.
 * Typically, it is retrieved in renderer behaviors' start() with a line like:
 *     m_transform = getComponent<TransformBehavior>();
 * This transform may be animated, and the animation altered by additional matrices
 * postTransform and postTransform. For animation, set "matrice" to e.g.
 *     { "buffer": "some_file.bin", "startFrame": 0 }
 * with some_file.bin containing a buffer of 32bit float matrices
 */
class TransformBehavior : public Behavior {
public:
	const glm::mat4 & modelMatrix() const { return m_modelMatrix; }

	const glm::mat4 & preTransform() const { return m_preTransform; }
	void setPreTransform(const glm::mat4 & transform) { m_preTransform = transform; updateModelMatrix(); }

	const glm::mat4 & postTransform() const { return m_postTransform; }
	void setPostTransform(const glm::mat4 & transform) { m_postTransform = transform; updateModelMatrix(); }

public:
	bool deserialize(const rapidjson::Value & json, const EnvironmentVariables & env, std::shared_ptr<AnimationManager> animations) override;

private:
	void updateModelMatrix();

private:
	glm::mat4 m_modelMatrix; // composited matrix
	glm::mat4 m_postTransform = glm::mat4(1);
	glm::mat4 m_transform;
	glm::mat4 m_preTransform = glm::mat4(1);
};

registerBehaviorType(TransformBehavior)
