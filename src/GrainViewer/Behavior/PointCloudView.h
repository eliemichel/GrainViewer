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

#include "IPointCloudData.h"
#include "PointCloudSplitter.h"

/**
 * Proxy to an externally allocated element buffer,
 * used by PointCloudSplitter to return sub parts of the original point cloud.
 * This is technically just a closure around PointCloudSplitter's method that
 * are a bit like IPointCloudData but with an extra model parameter.
 */
class PointCloudView : public IPointCloudData {
public:
	PointCloudView(const PointCloudSplitter& splitter, PointCloudSplitter::RenderModel model)
		: m_splitter(splitter), m_model(model) {}
	// IPointCloudData implementation
	GLsizei pointCount() const override { return m_splitter.pointCount(m_model); }
	GLsizei frameCount() const override { return m_splitter.frameCount(m_model); }
	GLuint vao() const override { return m_splitter.vao(m_model); }
	const GlBuffer& vbo() const override { return m_splitter.vbo(m_model); }
	std::shared_ptr<GlBuffer> ebo() const override { return m_splitter.ebo(m_model); }
	GLint pointOffset() const override { return m_splitter.pointOffset(m_model); }

private:
	const PointCloudSplitter& m_splitter;
	PointCloudSplitter::RenderModel m_model;
};

