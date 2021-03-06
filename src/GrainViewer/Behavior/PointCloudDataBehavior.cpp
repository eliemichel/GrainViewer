/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "PointCloudDataBehavior.h"
#include "PointCloud.h"
#include "ResourceManager.h"

#include "utils/strutils.h"
#include "Logger.h"

//-----------------------------------------------------------------------------
// Accessors

GLsizei PointCloudDataBehavior::pointCount() const
{
	return m_pointCount;
}

GLsizei PointCloudDataBehavior::frameCount() const
{
	return m_frameCount;
}

const GlBuffer & PointCloudDataBehavior::data() const
{
	return *m_pointBuffer;
}

GLuint PointCloudDataBehavior::vao() const
{
	return m_vao;
}

const GlBuffer & PointCloudDataBehavior::vbo() const
{
	return *m_pointBuffer;
}

//-----------------------------------------------------------------------------
// Behavior Implementation

bool PointCloudDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename")) {
		if (json["filename"].IsString()) {
			m_filename = json["filename"].GetString();
		} else {
			ERR_LOG << "Field 'filename' of PointCloudDataBehavior must be a string";
			return false;
		}
	}

	if (m_filename == "") {
		ERR_LOG << "MeshDataBehavior requires a filename to load";
		return false;
	}

	if (json.HasMember("bbox")) {
		if (json["bbox"].IsObject()) {
			m_useBbox = true;
			auto & bbox = json["bbox"];
			if (bbox.HasMember("xmin") && bbox["xmin"].IsNumber()) m_bboxMin.x = bbox["xmin"].GetFloat();
			if (bbox.HasMember("ymin") && bbox["ymin"].IsNumber()) m_bboxMin.y = bbox["ymin"].GetFloat();
			if (bbox.HasMember("zmin") && bbox["zmin"].IsNumber()) m_bboxMin.z = bbox["zmin"].GetFloat();
			if (bbox.HasMember("xmax") && bbox["xmax"].IsNumber()) m_bboxMax.x = bbox["xmax"].GetFloat();
			if (bbox.HasMember("ymax") && bbox["ymax"].IsNumber()) m_bboxMax.y = bbox["ymax"].GetFloat();
			if (bbox.HasMember("zmax") && bbox["zmax"].IsNumber()) m_bboxMax.z = bbox["zmax"].GetFloat();
		}
		else {
			ERR_LOG << "Field 'filename' of PointCloudDataBehavior must be a string";
			return false;
		}
	}

	m_filename = ResourceManager::resolveResourcePath(m_filename);

	return true;
}

void PointCloudDataBehavior::start()
{
	// 1. Initialize members

	m_pointBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);

	// 2. Load point cloud data

	PointCloud pointCloud;
	
	if (m_useBbox) {
		PointCloud fullPointCloud;
		fullPointCloud.load(m_filename);

		if (fullPointCloud.frameCount() > 1) {
			WARN_LOG << "Using bbox with animated point cloud has undefined behavior";
		}

		pointCloud.data().resize(128);
		int k = 0;
		for (const auto & p : fullPointCloud.data()) {
			if (k >= pointCloud.data().size()) {
				pointCloud.data().resize(2 * pointCloud.data().size());
			}

			if (p.x >= m_bboxMin.x && p.x <= m_bboxMax.x
				&& p.y >= m_bboxMin.y && p.y <= m_bboxMax.y
				&& p.z >= m_bboxMin.z && p.z <= m_bboxMax.z)
			{
				pointCloud.data()[k] = p;
				++k;
			}
		}
	}
	else {
		pointCloud.load(m_filename);
	}
	m_frameCount = static_cast<GLsizei>(pointCloud.frameCount());
	m_pointCount = static_cast<GLsizei>(pointCloud.data().size());

	// 3. Move data from PointCloud object to GlBuffer (in VRAM)

	m_pointBuffer->addBlock<glm::vec4>(m_pointCount);
	m_pointBuffer->addBlockAttribute(0, 4);  // position
	m_pointBuffer->alloc();
	m_pointBuffer->fillBlock<glm::vec4>(0, [&pointCloud](glm::vec4 *data, size_t _) {
		glm::vec4 *v = data;
		for (auto p : pointCloud.data()) {
			v->x = p.x; v->y = p.y; v->z = p.z; v++;
		}
	});

	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_pointBuffer->bind();
	m_pointBuffer->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_pointBuffer->finalize(); // This buffer will never be mapped on CPU
}

void PointCloudDataBehavior::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}
