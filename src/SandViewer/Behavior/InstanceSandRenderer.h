
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include "GlTexture.h"
#include "utils/ReflectionAttributes.h"

#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;
class SandBehavior;
class MeshDataBehavior;
class IPointCloudData;

/**
 * Render points from an IPointCloudData component by instancing the mesh from a MeshData component .
 * If a PointCloudSplitter component is available, use it's Instance sub-cloud.
 */
class InstanceSandRenderer : public Behavior {
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

	std::string m_shaderName = "InstanceSand";
	std::shared_ptr<ShaderProgram> m_shader;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<SandBehavior> m_sand;
	std::weak_ptr<MeshDataBehavior> m_mesh;
	std::weak_ptr<IPointCloudData> m_pointData;

	std::unique_ptr<GlTexture> m_colormapTexture;

	float m_time;
};

#define _ ReflectionAttributes::
REFL_TYPE(InstanceSandRenderer::Properties)
REFL_FIELD(grainMeshScale, _ Range(0.0f, 5.0f))
REFL_END
#undef _

registerBehaviorType(InstanceSandRenderer)
