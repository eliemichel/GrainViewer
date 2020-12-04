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

#include "MeshRenderer.h"
#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "MeshDataBehavior.h"
#include "ShaderPool.h"
#include "TransformBehavior.h"
#include "GlTexture.h"

#include "utils/jsonutils.h"
#include "utils/strutils.h"
#include "utils/behaviorutils.h"

#include "Logger.h"

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool MeshRenderer::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	jrArray(json, "materials", m_materials);
	return true;
}

void MeshRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_meshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();
}

void MeshRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	if (!m_shader->isValid()) return;

	if (auto mesh = m_meshData.lock()) {
		m_shader->use();

		glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
		m_shader->bindUniformBlock("Camera", camera.ubo());
		m_shader->setUniform("modelMatrix", modelMatrix());
		m_shader->setUniform("viewModelMatrix", viewModelMatrix);

		GLuint o = 0;
		int n = static_cast<int>(std::max(mesh->materials().size(), m_materials.size()));
		for (int i = 0 ; i < n ; ++i) {
			const StandardMaterial& mat = i < m_materials.size() ? m_materials[i] : mesh->materials()[i];
			o = mat.setUniforms(*m_shader, MAKE_STR("uMaterial[" << i << "]."), o);
		}

		autoSetUniforms(*m_shader, properties());

		glBindVertexArray(mesh->vao());
		glDrawArrays(GL_TRIANGLES, 0, mesh->pointCount());
		glBindVertexArray(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 MeshRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}