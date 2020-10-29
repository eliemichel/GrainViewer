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

#include <OpenGL>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"
#include "IPointCloudData.h"

#include <glm/glm.hpp>

#include <memory>

/**
 * Load point cloud from XYZ or adhoc BIN file to video memory. The later
 * can be animated.
 */
class PointCloudDataBehavior : public Behavior, public IPointCloudData {
public:
	// IPointCloudData implementation
	GLsizei pointCount() const override;
	GLsizei frameCount() const override;
	GLuint vao() const override;
	const GlBuffer & vbo() const override;

	const GlBuffer& data() const;

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	std::string m_filename = "";
	bool m_useBbox = false; // if true, remove all points out of the supplied bbox
	glm::vec3 m_bboxMin;
	glm::vec3 m_bboxMax;

	GLsizei m_pointCount;
	GLsizei m_frameCount;
	std::unique_ptr<GlBuffer> m_pointBuffer;
	GLuint m_vao;
};

registerBehaviorType(PointCloudDataBehavior)

