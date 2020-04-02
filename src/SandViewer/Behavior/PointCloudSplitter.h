#pragma once

#include <OpenGL>

#include "Behavior.h"
#include "GlBuffer.h"
#include "IPointCloudData.h"

#include <refl.hpp>

#include <memory>
#include <vector>

class ShaderProgram;
class PointCloudView;

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
	void onPreRender(const Camera& camera, const World& world, RenderType target) override;

public:
	
	enum class RenderTypeCaching {
		Forget,
		Cache,
		Precompute,
	};
	struct Properties {
		RenderTypeCaching renderTypeCaching = RenderTypeCaching::Forget;
	};
	enum class RenderModel {
		Instance = 0,
		Impostor,
		Point,
		None,
	};

	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

	// Return a point buffer for a given model
	std::shared_ptr<PointCloudView> subPointCloud(RenderModel model) const;

	GLint pointOffset(RenderModel model) const;
	GLsizei pointCount(RenderModel model) const;
	GLsizei frameCount(RenderModel model) const;
	GLuint vao(RenderModel model) const;

private:
	struct Counter {
		GLuint count = 0;
		GLuint offset = 0;
	};

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

private:
	Properties m_properties;

	std::string m_shaderName = "GlobalAtomic";
	mutable std::vector<std::shared_ptr<ShaderProgram>> m_shaders; // mutable for lazy loading, do NOT use this directly, rather use getShader()

	std::weak_ptr<IPointCloudData> m_pointData;

	std::unique_ptr<GlBuffer> m_elementBuffer;

	std::vector<Counter> m_counters;
	std::unique_ptr<GlBuffer> m_countersSsbo;

	GLuint m_elementCount;
	int m_local_size_x = 128;
	int m_xWorkGroups;
};

REFL_TYPE(PointCloudSplitter::Properties)
REFL_FIELD(renderTypeCaching)
REFL_END

/**
 * Proxy to an externally allocated element buffer,
 * used by PointCloudSplitter to return sub parts of the original point cloud.
 * This is technically just a closure around PointCloudSplitter's method that
 * are a bit like IPointCloudData but with an extra model parameter.
 */
class PointCloudView : public IPointCloudData {
public:
	PointCloudView(const PointCloudSplitter & splitter, PointCloudSplitter::RenderModel model)
		: m_splitter(splitter), m_model(model) {}
	// IPointCloudData implementation
	GLint pointOffset() const override { return m_splitter.pointOffset(m_model); }
	GLsizei pointCount() const override { return m_splitter.pointCount(m_model); }
	GLsizei frameCount() const override { return m_splitter.frameCount(m_model); }
	GLuint vao() const override { return m_splitter.vao(m_model); }

private:
	const PointCloudSplitter & m_splitter;
	PointCloudSplitter::RenderModel m_model;
};

registerBehaviorType(PointCloudSplitter)

