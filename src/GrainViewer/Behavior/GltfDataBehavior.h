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

#include "Behavior.h"

#include "tiny_gltf.h"

#include <memory>

class ShaderProgram;
class TransformBehavior;

/**
 * Load mesh from gltf file using tinygltf
 */
class GltfDataBehavior : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

private:
	glm::mat4 modelMatrix() const;

private:
	struct DrawCall {
		GLenum mode;
		GLsizei count;
		GLenum type;
		size_t byteOffset;
	};

	std::string m_filename;
	std::string m_shaderName = "GltfMesh";
	std::shared_ptr<ShaderProgram> m_shader;
	std::unique_ptr<tinygltf::Model> m_model;
	std::vector<GLuint> m_buffers;
	std::vector<DrawCall> m_drawCalls;
	std::vector<GLuint> m_vertexArrays;

	std::weak_ptr<TransformBehavior> m_transform;
};

registerBehaviorType(GltfDataBehavior)

