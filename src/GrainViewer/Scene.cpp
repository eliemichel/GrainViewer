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

#include "Scene.h"
#include "RuntimeObject.h"
#include "RenderType.h"
#include "ResourceManager.h"
#include "ShaderPool.h"
#include "Logger.h"
#include "AnimationManager.h"
#include "utils/fileutils.h"
#include "Filtering.h"
#include "GlDeferredShader.h"
#include "World.h"

#include <glm/glm.hpp>

#include <sstream>
#include <chrono>

#if _DEBUG
#include "utils/debug.h"
#endif

Scene::Scene()
	: m_viewportCameraIndex(0)
	, m_fps(25)
	, m_deferredShader(std::make_shared<GlDeferredShader>())
	, m_world(std::make_shared<World>())
{
	m_animationManager = std::make_shared<AnimationManager>();
	clear();
}

void Scene::setResolution(int width, int height)
{
	m_width = width;
	m_height = height;
	for (auto& camera : m_cameras) {
		if (camera->properties().displayInViewport) {
			camera->setResolution(width, height);
		}
	}
}

void Scene::reloadShaders() {
	ShaderPool::ReloadShaders();
	m_world->reloadShaders();
	m_deferredShader->reloadShaders();

	for (auto obj : m_objects) {
		obj->reloadShaders();
	}
}

void Scene::update(float time) {
	if (m_paused) {
		m_timeOffset = time - m_time;
	}
	else {
		m_time = time - m_timeOffset;
		++m_frameIndex;
	}

	
	if ((viewportCamera() && viewportCamera()->outputSettings().isRecordEnabled) || properties().realTime) {
		// If recording, slow down the game to ensure that it will play back correctly
		m_time = m_frameIndex / m_fps;
	} else {
		m_frameIndex = static_cast<int>(m_time * m_fps);
	}

	m_animationManager->update(m_time, m_frameIndex);

	for (auto& camera : m_cameras) {
		if (camera->properties().displayInViewport) {
			camera->update(m_time);
		}
	}

	m_world->update(m_time);

	for (auto obj : m_objects) {
		obj->update(m_time, m_frameIndex);
	}

	// Prepare output framebuffer
	if (viewportCamera()) {
		auto& s = viewportCamera()->outputSettings();
		if (!s.autoOutputResolution) {
			if (!m_outputFramebuffer || static_cast<int>(m_outputFramebuffer->width()) != s.width || static_cast<int>(m_outputFramebuffer->height()) != s.height) {
				LOG << "Rebuilding output framebuffer";
				const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
				m_outputFramebuffer = std::make_unique<Framebuffer>(s.width, s.height, colorLayerInfos);
			}
		}

		m_pixels.resize(3 * getOutputPixelCount(*viewportCamera()));
	}

	m_mustQuit = m_quitAfterFrame >= 0 && m_frameIndex > m_quitAfterFrame;

	// Occlusion camera freeze
	bool freeze = properties().freezeOcclusionCamera;
	if (freeze && !m_wasFreezeOcclusionCamera) {
		// copy viewport camera when freeze starts
		*occlusionCamera() = *viewportCamera();
		occlusionCamera()->properties().displayInViewport = false;
		occlusionCamera()->properties().controlInViewport = false;
	}
	m_wasFreezeOcclusionCamera = freeze;
}

void Scene::render() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);

	m_world->renderShadowMaps(m_objects);

	for (const auto& camera : m_cameras) {
		if (camera->properties().displayInViewport) {
			renderCamera(*camera);
		}
	}
}

void Scene::onPostRender(float time)
{
	if (viewportCamera()) {
		Camera & camera = *viewportCamera();
		recordFrame(camera);
		measureStats();
	}

	for (auto obj : m_objects) {
		obj->onPostRender(m_time, m_frameIndex);
	}
}

void Scene::clear()
{
	m_objects.clear();
	m_cameras.clear();
	m_world->clear();
	m_deferredShader = std::make_shared<GlDeferredShader>();
	m_animationManager->clear();
	m_frameIndex = -1;
}

std::shared_ptr<Camera> Scene::viewportCamera() const
{
	return m_viewportCameraIndex < m_cameras.size() ? m_cameras[m_viewportCameraIndex] : nullptr;
}

std::shared_ptr<Camera> Scene::occlusionCamera() const
{
	return m_occlusionCameraIndex < m_cameras.size() ? m_cameras[m_occlusionCameraIndex] : nullptr;
}

std::shared_ptr<RuntimeObject> Scene::findObjectByName(const std::string& name) {
	for (auto& obj : objects()) {
		if (obj->name == "OcclusionGeometry") {
			return obj;
		}
	}
	return nullptr;
}

void Scene::takeScreenshot() const
{
	using namespace std::chrono;
	std::time_t now = system_clock::to_time_t(system_clock::now());
	std::ostringstream ss;
	ss << "SandViewer_";
#ifdef WIN32
	tm t;
	localtime_s(&t, &now);
	ss << std::put_time(&t, "%Y-%m-%d_%H-%M-%S");
#else // WIN32
	ss << std::put_time(std::localtime(&now), "%Y-%m-%d_%H-%M-%S");
#endif // WIN32

	ss << ".exr";
	recordFrame(*viewportCamera(), ss.str(), RecordExr);
}

void Scene::play()
{
	m_paused = false;
}

void Scene::pause()
{
	m_paused = true;
}

void Scene::togglePause()
{
	m_paused = !m_paused;
}

bool Scene::isPaused() const
{
	return m_paused;
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
///////////////////////////////////////////////////////////////////////////////

void Scene::renderCamera(const Camera& camera) const
{
	const glm::vec2& res = camera.resolution();
	const glm::vec4& rect = camera.properties().viewRect;
	GLint vX0 = static_cast<GLint>(rect.x * m_width);
	GLint vY0 = static_cast<GLint>(rect.y * m_height);
	GLsizei vWidth = static_cast<GLsizei>(res.x);
	GLsizei vHeight = static_cast<GLsizei>(res.y);
	
	GLint vX = 0, vY = 0;
	if (m_isDeferredShadingEnabled) {
		m_deferredShader->bindFramebuffer(camera);
	}
	else if (camera.targetFramebuffer()) {
		camera.targetFramebuffer()->bind();
	}
	else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		vX = vX0;  vY = vY0;
	}

	glViewport(vX, vY, vWidth, vHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pre-rendering -- occlusion
	const Camera& prerenderCamera = properties().freezeOcclusionCamera ? *occlusionCamera() : camera;
	m_world->onPreRender(prerenderCamera);
	for (auto obj : m_objects) {
		if (obj->viewLayers & camera.properties().viewLayers) {
			obj->onPreRender(prerenderCamera, *m_world, RenderType::Default);
		}
	}

	// Main Rendering
	m_world->render(camera);
	for (auto obj : m_objects) {
		if (obj->viewLayers & camera.properties().viewLayers) {
			obj->render(camera, *m_world, RenderType::Default);
		}
	}

	if (m_isDeferredShadingEnabled) {
		if (camera.targetFramebuffer()) {
			camera.targetFramebuffer()->bind();
		}
		else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			vX = vX0;  vY = vY0;
			m_deferredShader->setBlitOffset(vX, vY);
		}
		glViewport(vX, vY, vWidth, vHeight);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_deferredShader->render(camera, *m_world, RenderType::Default);
	}

	if (camera.targetFramebuffer()) {
		GLint w1 = static_cast<GLint>(camera.resolution().x);
		GLint h1 = static_cast<GLint>(camera.resolution().y);
		float ratio = camera.resolution().y / camera.resolution().x;

		GLint w2 = static_cast<GLint>(m_width);
		GLint h2 = static_cast<GLint>(m_width * ratio);
		if (h2 > m_height) {
			w2 = static_cast<GLint>(m_height / ratio);
			h2 = static_cast<GLint>(m_height);
		}
		glBlitNamedFramebuffer(camera.targetFramebuffer()->raw(), 0, 0, 0, w1, h1, vX, vY, w2, h2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}
}

void Scene::measureStats()
{
	if (m_statsCountColors.empty() || !m_outputStatsFile.is_open()) return;
	std::vector<int> counters(m_statsCountColors.size());
	std::vector<uint8_t> intColors;
	for (const auto &c : m_statsCountColors) {
		intColors.push_back(static_cast<uint8_t>(c.x * 255.0f));
		intColors.push_back(static_cast<uint8_t>(c.y * 255.0f));
		intColors.push_back(static_cast<uint8_t>(c.z * 255.0f));
	}
	size_t n = m_pixels.size() / 3;
	size_t p = m_statsCountColors.size();
	for (size_t i = 0; i < n; ++i) {
		for (size_t k = 0; k < p; ++k) {
			if (intColors[3 * k + 0] == m_pixels[3 * i + 0]
				&& intColors[3 * k + 1] == m_pixels[3 * i + 1]
				&& intColors[3 * k + 2] == m_pixels[3 * i + 2]) {
				++counters[k];
			}
		}
	}

	m_outputStatsFile << m_frameIndex;
	for (const auto &c : counters) {
		m_outputStatsFile << ";" << c;
	}
	m_outputStatsFile << "\n";
}

size_t Scene::getOutputPixelCount(const Camera& camera) const
{
	auto& outputSettings = camera.outputSettings();
	size_t w = 0, h = 0;
	if (outputSettings.autoOutputResolution && camera.targetFramebuffer()) {
		w = static_cast<size_t>(camera.targetFramebuffer()->width());
		h = static_cast<size_t>(camera.targetFramebuffer()->height());
	}
	else if (outputSettings.autoOutputResolution) {
		w = static_cast<size_t>(m_width);
		h = static_cast<size_t>(m_height);
	}
	else if (m_outputFramebuffer) {
		w = static_cast<size_t>(m_outputFramebuffer->width());
		h = static_cast<size_t>(m_outputFramebuffer->height());
	}
	return w * h;
}

void Scene::recordFrame(const Camera & camera, const std::string & filename, RecordFormat format) const
{
	auto& outputSettings = camera.outputSettings();
	
	switch (format) {
	case RecordExr:
	{
		m_floatPixels.resize(3 * getOutputPixelCount(camera));

		const void* pixels = static_cast<const void*>(m_floatPixels.data());
		GLsizei bufSize = static_cast<GLsizei>(m_floatPixels.size() * sizeof(GLfloat));

		if (outputSettings.autoOutputResolution && camera.targetFramebuffer()) {
			ResourceManager::saveTexture_tinyexr(filename, camera.targetFramebuffer()->colorTexture(0));
		}
		else {
			if (!m_outputFramebuffer) {
				DEBUG_LOG << "alloc output fbo";
				// Even in autoOutputResolution, we need this framebuffer actually.
				const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
				m_outputFramebuffer = std::make_unique<Framebuffer>(m_width, m_height, colorLayerInfos);
			}

			// output dedicated output framebuffer
			GLuint sourceFbo = 0;
			GLint sourceWidth = m_width;
			GLint sourceHeight = m_height;
			if (camera.targetFramebuffer()) {
				sourceFbo = camera.targetFramebuffer()->raw();
				sourceWidth = static_cast<GLint>(camera.resolution().x);
				sourceHeight = static_cast<GLint>(camera.resolution().y);
			}
			GLint destWidth = static_cast<GLint>(m_outputFramebuffer->width());
			GLint destHeight = static_cast<GLint>(m_outputFramebuffer->height());
			glBlitNamedFramebuffer(sourceFbo, m_outputFramebuffer->raw(), 0, 0, sourceWidth, sourceHeight, 0, 0, destWidth, destHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			DEBUG_LOG << "save output fbo of size " << m_outputFramebuffer->width() << "x" << m_outputFramebuffer->height();
			ResourceManager::saveTexture_tinyexr(filename, m_outputFramebuffer->colorTexture(0));
		}
	}
	case RecordPng:
	default:
	{
		const void* pixels = static_cast<const void*>(m_pixels.data());
		GLsizei bufSize = static_cast<GLsizei>(m_pixels.size() * sizeof(uint8_t));

		if (outputSettings.autoOutputResolution && camera.targetFramebuffer()) {
			// output camera framebuffer
			camera.targetFramebuffer()->bind();
			GLint destWidth = static_cast<GLint>(camera.targetFramebuffer()->width());
			GLint destHeight = static_cast<GLint>(camera.targetFramebuffer()->height());
			glReadnPixels(0, 0, destWidth, destHeight, GL_RGB, GL_UNSIGNED_BYTE, bufSize, const_cast<void*>(pixels));
			if (outputSettings.saveOnDisc) {
				ResourceManager::saveImage(filename, destWidth, destHeight, 3, pixels);
			}
		}
		else if (outputSettings.autoOutputResolution) {
			// output default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glReadnPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, bufSize, const_cast<void*>(pixels));
			if (outputSettings.saveOnDisc) {
				ResourceManager::saveImage(filename, m_width, m_height, 3, pixels);
			}
		}
		else if (m_outputFramebuffer) {
			// output dedicated output framebuffer
			GLuint sourceFbo = 0;
			GLint sourceWidth = m_width;
			GLint sourceHeight = m_height;
			if (camera.targetFramebuffer()) {
				sourceFbo = camera.targetFramebuffer()->raw();
				sourceWidth = static_cast<GLint>(camera.resolution().x);
				sourceHeight = static_cast<GLint>(camera.resolution().y);
			}
			GLint destWidth = static_cast<GLint>(m_outputFramebuffer->width());
			GLint destHeight = static_cast<GLint>(m_outputFramebuffer->height());
			glBlitNamedFramebuffer(sourceFbo, m_outputFramebuffer->raw(), 0, 0, sourceWidth, sourceHeight, 0, destHeight, destWidth, 0, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			m_outputFramebuffer->bind();
			glReadnPixels(0, 0, destWidth, destHeight, GL_RGB, GL_UNSIGNED_BYTE, bufSize, const_cast<void*>(pixels));
			if (outputSettings.saveOnDisc) {
				ResourceManager::saveImage(filename, destWidth, destHeight, 3, pixels);
			}
		}
	}
	}
}

void Scene::recordFrame(const Camera & camera) const
{
	auto& outputSettings = camera.outputSettings();
	if (outputSettings.isRecordEnabled) {
		char number[7];
#ifdef _WIN32
		sprintf_s(number, 7, "%06d", m_frameIndex);
#else // _WIN32
		sprintf(number, "%04d", m_frameIndex);
#endif // _WIN32
		std::string filename = outputSettings.outputFrameBase + number + ".png";

		recordFrame(camera, filename, RecordPng);
	}
}
