#ifndef NO_GPL

#include "mono/Phase.hh" // sand6

#include "utils/strutils.h"
#include "utils/jsonutils.h"
#include "Logger.h"
#include "PointCloud.h"
#include "ResourceManager.h"
#include "Sand6Data.h"

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

GLsizei Sand6Data::pointCount() const
{
	return m_pointCount;
}

GLsizei Sand6Data::frameCount() const
{
	return m_frameCount;
}

const GlBuffer & Sand6Data::data() const
{
	return *m_pointBuffer;
}

GLuint Sand6Data::vao() const
{
	return m_vao;
}

const GlBuffer& Sand6Data::vbo() const
{
	return *m_pointBuffer;
}

///////////////////////////////////////////////////////////////////////////////
// Behavior Implementation
///////////////////////////////////////////////////////////////////////////////

bool Sand6Data::deserialize(const rapidjson::Value & json)
{
	if (jrOption(json, "directory", m_directory)) {
		m_directory = ResourceManager::resolveResourcePath(m_directory);
	} else {
		ERR_LOG << "Sand6Data requires a directory to load. This directory is the out/ of sand6's d6 binary";
		return false;
	}

	return true;
}

void Sand6Data::start()
{
	// 1. Initialize members

	m_pointBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);

	// 2. Load point cloud data

	d6::Offline offline(m_directory.c_str());
	const d6::Config& config = offline.config();

	offline.load_frame(1);
	DEBUG_LOG << "Loading " << offline.particles().count() << " particles from sand6 simulation";

	const Eigen::Matrix3Xd& centers = offline.particles().centers();

	m_frameCount = 1;
	m_pointCount = static_cast<GLsizei>(offline.particles().count());

	if (m_pointCount == 0) {
		ERR_LOG << "Sand6Data is empty";
		return;
	}

	// 3. Move data from PointCloud object to GlBuffer (in VRAM)

	m_pointBuffer->addBlock<glm::vec4>(m_pointCount);
	m_pointBuffer->addBlockAttribute(0, 4);  // position
	m_pointBuffer->alloc();
	m_pointBuffer->fillBlock<glm::vec4>(0, [&centers](glm::vec4 *data, size_t count) {
		glm::vec4 *v = data;
		for (size_t i = 0; i < count; ++i) {
			v->x = static_cast<float>(centers(0, i));
			v->y = static_cast<float>(centers(1, i));
			v->z = static_cast<float>(centers(2, i));
			v++;
		}
	});

	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_pointBuffer->bind();
	m_pointBuffer->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_pointBuffer->finalize(); // This buffer will never be mapped on CPU
}

void Sand6Data::onDestroy()
{
	glDeleteVertexArrays(1, &m_vao);
}

#endif // ndef NO_GPL
