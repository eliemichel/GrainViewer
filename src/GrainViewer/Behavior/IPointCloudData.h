#pragma once

#include <OpenGL>
#include "GlBuffer.h"
#include <memory>

/**
 * Interface common to all behaviors that can be used as point clouds by point-based renderers
 */
class IPointCloudData {
public:
	virtual GLsizei pointCount() const = 0;
	virtual GLsizei frameCount() const = 0;
	virtual GLuint vao() const = 0;
	virtual const GlBuffer& vbo() const = 0;
	virtual std::shared_ptr<GlBuffer> ebo() const { return nullptr; } // if null, then regular array is used as element buffer
	virtual GLint pointOffset() const { return 0; } // offset in the ebo
};
