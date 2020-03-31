#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/glad.h>
#include <memory>
#include <glm/glm.hpp>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"

/**
 * Interface common to all behaviors that can be used as point clouds by point-based renderers
 */
class IPointCloudData {
public:
	virtual GLsizei pointCount() const = 0;
	virtual GLsizei frameCount() const = 0;
	virtual const GlBuffer & data() const = 0;
	virtual GLuint vao() const = 0;
};
