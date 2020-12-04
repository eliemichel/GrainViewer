/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "GltfDataBehavior.h"
#include "ResourceManager.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"

#include "utils/strutils.h"
#include "Logger.h"

#include <functional>


static const std::vector<std::string> stdVertexAttribs{ "POSITION", "NORMAL", "TANGENT", "TEXCOORD_0", "TEXCOORD_1", "COLOR_0", "JOINTS_0", "WEIGHTS_0" };
#define BUFFER_OFFSET(offset) static_cast<void*>(static_cast<char*>(nullptr) + offset)

///////////////////////////////////////////////////////////////////////////////
// Behavior Implementation
///////////////////////////////////////////////////////////////////////////////

bool GltfDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename") && json["filename"].IsString()) {
		m_filename = ResourceManager::resolveResourcePath(json["filename"].GetString());
	} else {
		ERR_LOG << "GltfDataBehavior requires a filename to load";
		return false;
	}

	if (json.HasMember("shader")) {
		if (json["shader"].IsString()) {
			m_shaderName = json["shader"].GetString();
		}
		else {
			ERR_LOG << "GltfDataBehavior 'shader' parameter must be a string";
			return false;
		}
	}

	return true;
}

void GltfDataBehavior::start()
{
	// 1. Load GLTF
	LOG << "Loading " << m_filename << "...";
	m_model = std::make_unique<tinygltf::Model>();
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret = false;
	if (endsWith(m_filename, ".glb")) {
		ret = loader.LoadBinaryFromFile(m_model.get(), &err, &warn, m_filename.c_str());
	} else {
		ret = loader.LoadASCIIFromFile(m_model.get(), &err, &warn, m_filename.c_str());
	}
	if (!err.empty()) {
		ERR_LOG << "tinygltf: " << err;
	}
	if (!warn.empty()) {
		WARN_LOG << "tinygltf: " << warn;
	}
	if (!ret) {
		ERR_LOG << "Unable to open GLTF file: " << m_filename << " (see error above)";
		return;
	}
	LOG << "Loaded.";

	// 2. Move data to GlBuffer (in VRAM)

	auto& rootNode = m_model->nodes[m_model->scenes[m_model->defaultScene].nodes[0]];

	// Build VBO
	m_buffers.resize(m_model->buffers.size());
	glCreateBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
	DEBUG_LOG << "buffers:";
	for (int i = 0; i < m_model->buffers.size(); ++i) {
		const auto& b = m_model->buffers[i];
		DEBUG_LOG << " - " << b.name << ": " << b.data.size() << " bytes";
		glNamedBufferStorage(m_buffers[i], static_cast<GLsizeiptr>(b.data.size()), b.data.data(), NULL);
	}

	// Build VAOs and draw calls
	size_t primCount = 0;
	for (const auto& m : m_model->meshes) {
		primCount += m.primitives.size();
	}
	DEBUG_LOG << "Found " << primCount << " primitives";
	m_drawCalls.resize(primCount);
	m_vertexArrays.resize(primCount);
	glCreateVertexArrays(static_cast<GLsizei>(m_vertexArrays.size()), m_vertexArrays.data());
	
	DEBUG_LOG << "bufferViews:";
	for (int i = 0; i < m_model->bufferViews.size(); ++i) {
		const auto& v = m_model->bufferViews[i];
		DEBUG_LOG << " - " << v.name << ": buffer #" << v.buffer << " from offset " << v.byteOffset << " for " << v.byteLength << " bytes by a stride of " << v.byteStride;
		for (auto vao : m_vertexArrays) {
			glVertexArrayVertexBuffer(vao, i, m_buffers[v.buffer], static_cast<GLintptr>(v.byteOffset), static_cast<GLsizei>(v.byteStride));
		}
	}

	DEBUG_LOG << "meshes:";
	int primId = 0;
	for (int i = 0; i < m_model->meshes.size(); ++i) {
		const auto& m = m_model->meshes[i];
		DEBUG_LOG << " - " << m.name << ": ";
		for (int j = 0; j < m.primitives.size(); ++j) {
			const auto& p = m.primitives[j];
			auto& drawCall = m_drawCalls[primId];
			auto& vao = m_vertexArrays[primId++];

			std::ostringstream ss;
			for (const auto & attr : p.attributes) {
				if (!ss.str().empty()) ss << ", ";
				ss << attr.first << ": " << attr.second;
			}
			DEBUG_LOG << "   * primitive(attributes={" << ss.str() << "}, indices=#" << p.indices << ")";

			for (int k = 0 ; k < stdVertexAttribs.size() ; ++k) {
				const std::string & attribName = stdVertexAttribs[k];
				if (p.attributes.count(attribName)) {
					DEBUG_LOG << "        (contains std attrib " << attribName << ")";
					const auto& a = m_model->accessors[p.attributes.at(attribName)];

					glEnableVertexArrayAttrib(vao, k);
#if 0 // unfortunately GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSETis too low for the "modern way" to work (weird)
					glVertexArrayAttribBinding(vao, k, a.bufferView);
					glVertexArrayAttribFormat(vao, k, static_cast<GLint>(a.type % 16), a.componentType, a.normalized ? GL_TRUE : GL_FALSE, static_cast<GLuint>(a.byteOffset));
#else				
					const auto& v = m_model->bufferViews[a.bufferView];
					glBindVertexArray(vao);
					glBindBuffer(GL_ARRAY_BUFFER, m_buffers[v.buffer]);
					glVertexAttribPointer(k, static_cast<GLint>(a.type % 16), a.componentType, a.normalized ? GL_TRUE : GL_FALSE, a.ByteStride(v), BUFFER_OFFSET(v.byteOffset + a.byteOffset));
#endif
				}
				else {
					glDisableVertexArrayAttrib(vao, k);
				}
			}
			// Elements
			const auto& a = m_model->accessors[p.indices];
			const auto& v = m_model->bufferViews[a.bufferView];
			glVertexArrayElementBuffer(vao, m_buffers[v.buffer]); // what about buffer view attributes?
			glVertexArrayBindingDivisor(vao, a.bufferView, static_cast<GLuint>(v.byteStride));
			drawCall.mode = p.mode;
			drawCall.count = static_cast<GLsizei>(a.count);
			drawCall.type = a.componentType;
			drawCall.byteOffset = v.byteOffset;
		}
	}

	// 3. Free mesh from RAM now that it is in VRAM
	m_model.reset();

	// 4. Misc
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_transform = getComponent<TransformBehavior>();
}

void GltfDataBehavior::onDestroy()
{
	glDeleteBuffers(static_cast<GLsizei>(m_buffers.size()), m_buffers.data());
	glDeleteVertexArrays(static_cast<GLsizei>(m_vertexArrays.size()), m_vertexArrays.data());
}

void GltfDataBehavior::render(const Camera& camera, const World& world, RenderType target) const
{
	m_shader->use();
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	m_shader->bindUniformBlock("Camera", camera.ubo());
	m_shader->setUniform("modelMatrix", modelMatrix());

	m_shader->setUniform("viewModelMatrix", viewModelMatrix);
	for (int i = 0; i < m_vertexArrays.size(); ++i) {
		const auto& drawCall = m_drawCalls[i];
		glBindVertexArray(m_vertexArrays[i]);
		glDrawElements(drawCall.mode, drawCall.count, drawCall.type, BUFFER_OFFSET(drawCall.byteOffset));
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 GltfDataBehavior::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	}
	else {
		return glm::mat4(1.0f);
	}
}
