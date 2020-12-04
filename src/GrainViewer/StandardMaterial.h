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
#include <tiny_obj_loader.h>

#include <string>
#include <memory>

class ShaderProgram;

/**
 * A material is a set of attributes that can be used by rendering behaviors.
 */
struct StandardMaterial
{
	glm::vec3 baseColor = glm::vec3(1.0f, 0.5f, 0.0f);
	GLfloat metallic = 0.0f;
	GLfloat roughness = 0.5f;
	std::unique_ptr<GlTexture> baseColorMap;
	std::unique_ptr<GlTexture> normalMap;
	std::unique_ptr<GlTexture> metallicRoughnessMap;
	std::unique_ptr<GlTexture> metallicMap;
	std::unique_ptr<GlTexture> roughnessMap;

	bool deserialize(const rapidjson::Value& json);
	void fromTinyObj(const tinyobj::material_t & mat, const std::string& textureRoot);
	// return the next available texture unit
	GLuint setUniforms(const ShaderProgram& shader, const std::string & prefix, GLuint nextTextureUnit) const;
};
