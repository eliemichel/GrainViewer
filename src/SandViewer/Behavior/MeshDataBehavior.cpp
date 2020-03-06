#include "Logger.h"

#include "bufferFillers.h"
#include "ResourceManager.h"
#include "MeshDataBehavior.h"

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

GLsizei MeshDataBehavior::pointCount() const
{
	return m_pointCount;
}

GLuint MeshDataBehavior::vao() const
{
	return m_vao;
}

///////////////////////////////////////////////////////////////////////////////
// Behavior Implementation
///////////////////////////////////////////////////////////////////////////////

bool MeshDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename")) {
		if (json["filename"].IsString()) {
			m_filename = json["filename"].GetString();
		} else {
			ERR_LOG << "Field 'filename' of MeshDataBehavior must be a string";
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

void MeshDataBehavior::start()
{
	// 1. Initialize members

	m_mesh = std::make_unique<Mesh>(m_filename);
	m_vertexBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);

	// 2. Move data from Mesh object to GlBuffer (in VRAM)

	m_pointCount = 0;
	for (auto s : m_mesh->shapes()) {
		m_pointCount += static_cast<GLsizei>(s.mesh.indices.size());
	}

	// Build VBO
	m_vertexBuffer->addBlock<PointAttributes>(m_pointCount);
	m_vertexBuffer->addBlockAttribute(0, 3);  // position
	m_vertexBuffer->addBlockAttribute(0, 3);  // normal
	m_vertexBuffer->addBlockAttribute(0, 2);  // uv
	m_vertexBuffer->addBlockAttribute(0, 1);  // materialId
	m_vertexBuffer->addBlockAttribute(0, 3);  // tangent
	m_vertexBuffer->alloc();
	m_vertexBuffer->fillBlock(0, fillPointAttributes, *m_mesh);

	// Build VAO
	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_vertexBuffer->bind();
	m_vertexBuffer->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_vertexBuffer->finalize();

	// 3. Free mesh from RAM now that it is in VRAM

	m_mesh.reset();
}

void MeshDataBehavior::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}
