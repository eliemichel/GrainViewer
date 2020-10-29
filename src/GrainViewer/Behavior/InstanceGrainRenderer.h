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

#include <OpenGL>
#include "Behavior.h"
#include "GlTexture.h"
#include "utils/ReflectionAttributes.h"
#include "StandardMaterial.h"

#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;
class GrainBehavior;
class MeshDataBehavior;
class IPointCloudData;

/**
 * Render points from an IPointCloudData component by instancing the mesh from a MeshData component .
 * If a PointCloudSplitter component is available, use it's Instance sub-cloud.
 */
class InstanceGrainRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

public:
	// Properties (serialized and displayed in UI)
	struct Properties {
		// Correct scale of the MeshData mesh when using it as instance.
		float grainMeshScale = 1.0f;
	};
	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

private:
	Properties m_properties;

	std::string m_shaderName = "InstanceGrain";
	std::shared_ptr<ShaderProgram> m_shader;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<GrainBehavior> m_grain;
	std::weak_ptr<MeshDataBehavior> m_mesh;
	std::weak_ptr<IPointCloudData> m_pointData;

	std::unique_ptr<GlTexture> m_colormapTexture;
	std::vector<StandardMaterial> m_materials; // may be emtpy, in which case materials from MeshData are used

	float m_time;
};

#define _ ReflectionAttributes::
REFL_TYPE(InstanceGrainRenderer::Properties)
REFL_FIELD(grainMeshScale, _ Range(0.0f, 5.0f))
REFL_END
#undef _

registerBehaviorType(InstanceGrainRenderer)
