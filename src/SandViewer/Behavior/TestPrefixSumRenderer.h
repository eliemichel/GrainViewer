#pragma once

#include <memory>
#include "Behavior.h"
#include "PointCloud.h"
#include "GlTexture.h"
#include "GlBuffer.h"

class ShaderProgram;

/**
 * Test compute shader that performs prefix sum
 */
class TestPrefixSumRenderer : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void update(float time);
	void render(const Camera & camera, const World & world, RenderType target) const override;

private:
	std::string m_shaderName = "PrefixSum";
	int m_pointCount;
	int m_frame = 0;

	std::shared_ptr<ShaderProgram> m_shader;
	std::unique_ptr<GlBuffer> m_elementBuffer1;
	std::unique_ptr<GlBuffer> m_elementBuffer2;
};
