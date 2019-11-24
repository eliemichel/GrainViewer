// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include "Scene.h"
#include "RuntimeObject.h"
#include "RenderType.h"
#include "ShaderPool.h"

#if _DEBUG
#include "utils/debug.h"
#endif

#include <glm/glm.hpp>

using glm::vec3;
using glm::mat4;

using namespace std;

Scene::Scene()
{}

void Scene::reloadShaders() {
	ShaderPool::ReloadShaders();
	m_world.reloadShaders();

	for (auto obj : m_objects) {
		obj->reloadShaders();
	}
}

void Scene::update(float time) {
	m_viewportCamera.update(time);

	for (auto obj : m_objects) {
		obj->update(time);
	}
}

void Scene::render() const {
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const glm::vec2 & res = m_viewportCamera.resolution();
	glViewport(0, 0, static_cast<GLsizei>(res.x), static_cast<GLsizei>(res.y));
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_world.render(m_viewportCamera);
	for (auto obj : m_objects) {
		obj->render(m_viewportCamera, m_world, DefaultRendering);
	}
}
