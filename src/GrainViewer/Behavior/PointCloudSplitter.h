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
#include "GlBuffer.h"
#include "IPointCloudData.h"
#include "utils/ReflectionAttributes.h"

#include <refl.hpp>

#include <fstream>
#include <memory>
#include <vector>

class ShaderProgram;
class PointCloudView;
class TransformBehavior;
class GrainBehavior;

/**
 * The Point Cloud Splitter behavior uses the preRender pass to split
 * the point cloud into contiguous element buffers for each rendering model.
 * This component must be placed *after* point data.
 */
class PointCloudSplitter : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time, int frame) override;
	void onPreRender(const Camera& camera, const World& world, RenderType target) override;

public:
	enum class RenderTypeCaching {
		Forget, // Uses less memory
		Cache, // Faster, but by max 1%...
		Precompute, // Not recommended
	};
	struct Properties {
		RenderTypeCaching renderTypeCaching = RenderTypeCaching::Cache;
		bool enableOcclusionCulling = true;
		bool enableFrustumCulling = true;
		float instanceLimit = 1.05f; // distance beyond which we switch from instances to impostors
		float impostorLimit = 10.0f;
		bool zPrepass = true; // for occluder map
		bool useBbox = false; // if true, remove all points out of the supplied bounding box
		glm::vec3 bboxMin;
		glm::vec3 bboxMax;
		float occluderMapSpriteScale = 0.2f;
	};
	enum class RenderModel {
		Instance = 0,
		Impostor,
		Point,
		None,
	};

	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

	struct Counter {
		GLuint count = 0;
		GLuint offset = 0;
	};
	const std::vector<Counter> counters() const { return m_counters; }

	// Return a point buffer for a given model
	std::shared_ptr<PointCloudView> subPointCloud(RenderModel model) const;
	GLsizei pointCount(RenderModel model) const;
	GLsizei frameCount(RenderModel model) const;
	GLuint vao(RenderModel model) const;
	const GlBuffer& vbo(RenderModel model) const;
	std::shared_ptr<GlBuffer> ebo(RenderModel model) const;
	GLint pointOffset(RenderModel model) const;

private:
	glm::mat4 modelMatrix() const;
	void setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const;

	// These must match defines in the shader (magic_enum reflexion is used to set defines)
	// the first one mirrors RenderTypeCaching (which is for diaplay)
	enum class RenderTypeShaderVariant {
		RENDER_TYPE_FORGET,
		RENDER_TYPE_CACHE,
		RENDER_TYPE_PRECOMPUTE,
	};
	enum class StepShaderVariant {
		STEP_PRECOMPUTE,
		STEP_RESET,
		STEP_COUNT,
		STEP_OFFSET,
		STEP_WRITE,
	};
	typedef int ShaderVariantFlagSet;
	std::shared_ptr<ShaderProgram> getShader(RenderTypeCaching renderType, int step) const; // for convenience
	std::shared_ptr<ShaderProgram> getShader(RenderTypeShaderVariant renderType, StepShaderVariant step) const;

	void initStats();
	void writeStats();

private:
	Properties m_properties;

	std::string m_shaderName = "GlobalAtomic";
	std::string m_occlusionCullingShaderName = "OcclusionCulling";
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()
	std::shared_ptr<ShaderProgram> m_occlusionCullingShader;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<GrainBehavior> m_grain;
	std::weak_ptr<IPointCloudData> m_pointData;

	std::shared_ptr<GlBuffer> m_elementBuffer; // must be shared because exposed through IPointCloudData interface
	mutable std::unique_ptr<GlBuffer> m_renderTypeCache; // lazily allocated

	std::vector<Counter> m_counters;
	std::unique_ptr<GlBuffer> m_countersSsbo;

	// Output subclouds
	std::vector<std::shared_ptr<PointCloudView>> m_subClouds;

	GLuint m_elementCount;
	int m_local_size_x = 128;
	int m_xWorkGroups;
	float m_time;

	// stats
	std::string m_outputStats;
	std::ofstream m_outputStatsFile;
	int m_statFrame = 0;
};

#define _ ReflectionAttributes::
REFL_TYPE(PointCloudSplitter::Properties)
REFL_FIELD(renderTypeCaching, _ HideInDialog())
REFL_FIELD(enableOcclusionCulling)
REFL_FIELD(enableFrustumCulling)
REFL_FIELD(instanceLimit, _ Range(0.01f, 3.0f))
REFL_FIELD(impostorLimit, _ Range(0.01f, 20.0f))
REFL_FIELD(zPrepass)
REFL_FIELD(useBbox)
REFL_FIELD(bboxMin, _ Range(-1, 1))
REFL_FIELD(bboxMax, _ Range(-1, 1))
REFL_FIELD(occluderMapSpriteScale)
REFL_END
#undef _

registerBehaviorType(PointCloudSplitter)
