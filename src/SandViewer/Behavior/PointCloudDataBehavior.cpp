#include "utils/strutils.h"
#include "Logger.h"
#include "PointCloud.h"
#include "ResourceManager.h"
#include "PointCloudDataBehavior.h"

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

GLsizei PointCloudDataBehavior::pointCount() const
{
	return m_pointCount;
}

GLsizei PointCloudDataBehavior::frameCount() const
{
	return m_frameCount;
}

const GlBuffer & PointCloudDataBehavior::data() const
{
	return *m_pointBuffer;
}

GLuint PointCloudDataBehavior::vao() const
{
	return m_vao;
}

///////////////////////////////////////////////////////////////////////////////
// Behavior Implementation
///////////////////////////////////////////////////////////////////////////////

bool PointCloudDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename")) {
		if (json["filename"].IsString()) {
			m_filename = json["filename"].GetString();
		} else {
			ERR_LOG << "Field 'filename' of PointCloudDataBehavior must be a string";
			return false;
		}
	}

	if (m_filename == "") {
		ERR_LOG << "MeshDataBehavior requires a filename to load";
		return false;
	}

	m_filename = ResourceManager::resolveResourcePath(m_filename);

	return true;
}

void PointCloudDataBehavior::start()
{
	// 1. Initialize members

	m_pointBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);

	// 2. Load point cloud data

	PointCloud pointCloud;
	if (endsWith(m_filename, ".bin")) {
		pointCloud.loadBin(m_filename);
	} else {
		pointCloud.loadXYZ(m_filename);
	}
	m_pointCount = static_cast<GLsizei>(pointCloud.data().size());
	m_frameCount = static_cast<GLsizei>(pointCloud.frameCount());

	// 3. Move data from PointCloud object to GlBuffer (in VRAM)

	m_pointBuffer->addBlock<glm::vec4>(m_pointCount);
	m_pointBuffer->addBlockAttribute(0, 4);  // position
	m_pointBuffer->alloc();
	m_pointBuffer->fillBlock<glm::vec4>(0, [&pointCloud](glm::vec4 *data, size_t _) {
		glm::vec4 *v = data;
		for (auto p : pointCloud.data()) {
			v->x = p.x; v->y = p.y; v->z = p.z; v++;
		}
	});

	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_pointBuffer->bind();
	m_pointBuffer->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_pointBuffer->finalize(); // This buffer will never be mapped on CPU
}

void PointCloudDataBehavior::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}
