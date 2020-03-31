#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/glad.h>
#include <memory>
#include <glm/glm.hpp>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"
#include "IPointCloudData.h"

/**
 * Load point cloud from XYZ or adhoc BIN file to video memory
 */
class PointCloudDataBehavior : public Behavior, public IPointCloudData {
public:
	// IPointCloudData implementation
	GLsizei pointCount() const override;
	GLsizei frameCount() const override;
	const GlBuffer& data() const override;
	GLuint vao() const override;

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	std::string m_filename = "";
	bool m_useBbox = false; // if true, remove all points out of the supplied bbox
	glm::vec3 m_bboxMin;
	glm::vec3 m_bboxMax;

	GLsizei m_pointCount;
	GLsizei m_frameCount;
	std::unique_ptr<GlBuffer> m_pointBuffer;
	GLuint m_vao;
};

registerBehaviorType(PointCloudDataBehavior)

