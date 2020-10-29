/**
 * This file is part of GrainViewer
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

#include "Behavior.h"
#include "ShaderProgram.h"
#include "GlTexture.h"
#include "StandardMaterial.h"

#include <refl.hpp>
#include <glm/glm.hpp>

#include <memory>

class MeshDataRenderer;
class TransformBehavior;
class MeshDataBehavior;

/**
 * Regular mesh renderer, rendering mesh from MeshDataBehavior
 */
class MeshRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

public:
	struct Properties {
		float normalMapping = 1.0f;
	};
	Properties& properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;
	std::string m_shaderName = "Mesh";
	std::weak_ptr<MeshDataBehavior> m_meshData;
	std::weak_ptr<TransformBehavior> m_transform;
	std::shared_ptr<ShaderProgram> m_shader;
	std::vector<StandardMaterial> m_materials; // may be emtpy, in which case materials from MeshData are used
};

#define _ ReflectionAttributes::
REFL_TYPE(MeshRenderer::Properties)
REFL_FIELD(normalMapping, _ Range(0, 2))
REFL_END
#undef _

registerBehaviorType(MeshRenderer)
