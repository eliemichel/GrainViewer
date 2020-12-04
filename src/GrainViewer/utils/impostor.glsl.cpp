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
