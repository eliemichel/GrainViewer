#include "MeshDataBehavior.h"
#include "TransformBehavior.h"
#include "bufferFillers.h"
#include "ResourceManager.h"
#include "utils/fileutils.h"
#include "Logger.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
namespace fs = std::filesystem;

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

	if (json.HasMember("computeBoundingSphere")) {
		if (json["computeBoundingSphere"].IsBool()) {
			m_computeBoundingSphere = json["computeBoundingSphere"].GetBool();
		}
		else {
			ERR_LOG << "Field 'computeBoundingSphere' of MeshDataBehavior must be a boolean";
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

	// 3. Compute optional statistics
	if (m_computeBoundingSphere) {
		computeBoundingSphere();
	}

	// 4. Get materials
	std::string textureRoot = fs::absolute(filename()).parent_path().string();
	m_materials.resize(m_mesh->materials().size());
	for (int i = 0; i < m_materials.size(); ++i) {
		m_materials[i].fromTinyObj(m_mesh->materials()[i], textureRoot);
	}

	// 5. Free mesh from RAM now that it is in VRAM
	m_mesh.reset();
}

void MeshDataBehavior::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}

void MeshDataBehavior::computeBoundingSphere()
{
	// This is an approximation, but reasonable enough.

	glm::mat4 matrix = glm::mat4(1.0f);
	if (auto transform = getComponent<TransformBehavior>().lock()) {
		matrix = transform->modelMatrix();
	}

	// 1. Compute AABB
	glm::vec3 bb_min(+99999.9f);
	glm::vec3 bb_max(-99999.9f);

	const std::vector<float> & v = m_mesh->attrib().vertices;
	for (auto s : m_mesh->shapes()) {
		const auto & indices = s.mesh.indices;
		for (size_t i = 0; i < indices.size(); ++i) {
			glm::vec3 P = matrix * glm::vec4(glm::make_vec3(&v[3 * indices[i].vertex_index]), 1.0);
			bb_min = glm::min(bb_min, P);
			bb_max = glm::max(bb_max, P);
		}
	}

	// 2. Get radius
	m_boundingSphereCenter = (bb_min + bb_max) / 2.0f;
	m_boundingSphereRadius = 0.0f;

	for (auto s : m_mesh->shapes()) {
		const auto & indices = s.mesh.indices;
		for (size_t i = 0; i < indices.size(); ++i) {
			glm::vec3 P = matrix * glm::vec4(glm::make_vec3(&v[3 * indices[i].vertex_index]), 1.0);
			m_boundingSphereRadius = glm::max(m_boundingSphereRadius, glm::length(P - m_boundingSphereCenter));
		}
	}
}
