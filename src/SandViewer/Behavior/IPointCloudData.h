#pragma once

#include <OpenGL>

/**
 * Interface common to all behaviors that can be used as point clouds by point-based renderers
 */
class IPointCloudData {
public:
	virtual GLint pointOffset() const { return 0; }
	virtual GLsizei pointCount() const = 0;
	virtual GLsizei frameCount() const = 0;
	virtual GLuint vao() const = 0;
};
