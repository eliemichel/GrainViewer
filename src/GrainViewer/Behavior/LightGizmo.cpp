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

#include "LightGizmo.h"
#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "Light.h"
#include "ShaderPool.h"
#include "GlBuffer.h"

#include "utils/jsonutils.h"
#include "Logger.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


bool LightGizmo::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);

	if (!jrOption(json, "light", m_lightIndex)) {
		ERR_LOG << "Field 'light' of LightGizmo must be an integer index";
		return false;
	}

	return true;
}

void LightGizmo::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_vertexBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);

	// Build VBO
	m_vertexBuffer->addBlock<glm::vec3>(14);
	m_vertexBuffer->addBlockAttribute(0, 3);  // position
	m_vertexBuffer->alloc();
	m_vertexBuffer->fillBlock<glm::vec3>(0, [](glm::vec3 *data, size_t _) {
		// Cube in triangle strip
		glm::vec3 *p;
		p = data + 0; p->x = 0; p->y = 1; p->z = 1;
		p = data + 1; p->x = 1; p->y = 1; p->z = 1;
		p = data + 2; p->x = 0; p->y = 0; p->z = 1;
		p = data + 3; p->x = 1; p->y = 0; p->z = 1;
		p = data + 4; p->x = 1; p->y = 0; p->z = 0;
		p = data + 5; p->x = 1; p->y = 1; p->z = 1;
		p = data + 6; p->x = 1; p->y = 1; p->z = 0;
		p = data + 7; p->x = 0; p->y = 1; p->z = 1;
		p = data + 8; p->x = 0; p->y = 1; p->z = 0;
		p = data + 9; p->x = 0; p->y = 0; p->z = 1;
		p = data + 10; p->x = 0; p->y = 0; p->z = 0;
		p = data + 11; p->x = 1; p->y = 0; p->z = 0;
		p = data + 12; p->x = 0; p->y = 1; p->z = 0;
		p = data + 13; p->x = 1; p->y = 1; p->z = 0;
	});
	
	// Build VAO
	glCreateVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);
	m_vertexBuffer->bind();
	m_vertexBuffer->enableAttributes(m_vao);
	glBindVertexArray(0);

	m_vertexBuffer->finalize();
}

void LightGizmo::render(const Camera& camera, const World& world, RenderType target) const
{
	if (target == RenderType::ShadowMap) return;
	if (!m_shader->isValid()) return;

	const auto& lights = world.lights();
	if (m_lightIndex < 0 || m_lightIndex >= lights.size()) {
		return;
	}

	const Camera & projector = lights[m_lightIndex]->shadowMap().camera();

	m_shader->use();

	glm::mat4 modelMatrix(1.0f);
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix;
	m_shader->bindUniformBlock("Camera", camera.ubo());
	m_shader->setUniform("modelMatrix", modelMatrix);
	m_shader->setUniform("viewModelMatrix", viewModelMatrix);

	m_shader->setUniform("lightProjectionMatrix", projector.projectionMatrix());
	m_shader->setUniform("lightInverseViewMatrix", glm::inverse(projector.viewMatrix()));

	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
	glBindVertexArray(0);
}
