#include "impostor.glsl.h"

// Extracted from impostor.inc.glsl
namespace glsl {
	using namespace glm;

	/**
	 * Return the direction of the i-th plane in an octahedric division of the unit
	 * sphere of n subdivisions.
	 */
	vec3 ViewIndexToDirection(uint i, uint n) {
		float eps = -1;
		uint n2 = n * n;
		if (i >= n2) {
			eps = 1;
			i -= n2;
		}
		vec2 uv = vec2(i / n, i % n) / float(n - 1);
		float x = uv.x + uv.y - 1;
		float y = uv.x - uv.y;
		float z = eps * (1.0f - abs(x) - abs(y));
		// break symmetry in redundant parts. TODO: find a mapping without redundancy, e.g. full octahedron
		if (z == 0) z = 0.0001f * eps;
		return normalize(vec3(x, y, z));
	}

	/**
	 * Build the inverse of the view matrix used to bake the i-th G-Impostor
	 */
	mat4 InverseBakingViewMatrix(uint i, uint n) {
		vec3 z = ViewIndexToDirection(i, n);
		vec3 x = normalize(cross(vec3(0, 0, 1), z));
		vec3 y = normalize(cross(z, x));
		vec3 w = vec3(0);
		return transpose(mat4(
			vec4(x, 0),
			vec4(y, 0),
			vec4(z, 0),
			vec4(w, 1)
		));
	}
}
