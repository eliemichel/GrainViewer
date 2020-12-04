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

#pragma once

#include <OpenGL>
#include "GlTexture.h"

#include <glm/glm.hpp>
#include <rapidjson/document.h>

#include <string>
#include <memory>

class ShaderProgram;
class MeshDataBehavior;

/**
 * Matches SphericalImpostor struct in include/impostor.inc.glsl
 */
struct ImpostorAtlasMaterial
{
	std::unique_ptr<GlTexture> normalAlphaTexture;
	std::unique_ptr<GlTexture> baseColorTexture;
	std::unique_ptr<GlTexture> metallicRoughnessTexture;
	glm::vec3 baseColor;
	float metallic = 0.0;
	float roughness = 0.5;

	GLuint viewCount; // number of precomputed views, computed from normalAlphaTexture depth

	bool bake = false;
	// If 'bake' is true, impostor is computed in engine at startup rather than being loaded from files.
	// in this case, the following options are used:
	int angularDefinition = 128; // rounded to the closest number such that 2n�
	int spatialDefinition = 128; // number of pixels on each dimension of a precomputed view

	bool deserialize(const rapidjson::Value& json);
	GLint setUniforms(const ShaderProgram& shader, const std::string& prefix, GLint nextTextureUnit) const;

private:
	void bakeMaps(const MeshDataBehavior& mesh, float radius, glm::vec3 center);
};
