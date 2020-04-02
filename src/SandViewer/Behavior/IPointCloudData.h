#pragma once

#include <OpenGL>
#include "GlBuffer.h"

/**
 * Interface common to all behaviors that can be used as point clouds by point-based renderers
 */
class IPointCloudData {
public:
	virtual GLint pointOffset() const { return 0; }
	virtual GLsizei pointCount() const = 0;
	virtual GLsizei frameCount() const = 0;
	virtual GLuint vao() const = 0;
	virtual const GlBuffer& vbo() const = 0;
};
