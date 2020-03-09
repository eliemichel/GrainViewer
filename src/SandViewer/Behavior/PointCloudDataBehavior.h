#pragma once

#include <glad/glad.h>
#include <memory>
#include <glm/glm.hpp>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"

/**
 * Load point cloud from XYZ or adhoc BIN file to video memory
 */
class PointCloudDataBehavior : public Behavior {
public:
	// Accessors
	GLsizei pointCount() const;
	GLsizei frameCount() const;
	const GlBuffer & data() const;
	GLuint vao() const;

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	std::string m_filename = "";

	GLsizei m_pointCount;
	GLsizei m_frameCount;
	std::unique_ptr<GlBuffer> m_pointBuffer;
	GLuint m_vao;
};

registerBehaviorType(PointCloudDataBehavior)
