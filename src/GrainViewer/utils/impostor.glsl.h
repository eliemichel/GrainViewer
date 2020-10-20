#pragma once

#include <glm/glm.hpp>

// (almost) mutant code replicated from impostor.inc.glsl
// comments are in the cpp

namespace glsl {
	using namespace glm;

	mat4 InverseBakingViewMatrix(uint i, uint n);
}

