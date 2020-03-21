// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#include <sstream>

#include <glm/glm.hpp>

#include "Scene.h"
#include "RuntimeObject.h"
#include "RenderType.h"
#include "ResourceManager.h"
#include "ShaderPool.h"
#include "Logger.h"
#include "AnimationManager.h"
#include "utils/fileutils.h"
#include "Filtering.h"

#if _DEBUG
#include "utils/debug.h"
#endif

using namespace std;

Scene::Scene()
	: m_viewportCameraIndex(0)
	, m_fps(25)
{
	m_animationManager = std::make_shared<AnimationManager>();
	clear();
}

void Scene::setResolution(int width, int height)
{
	m_width = width;
	m_height = height;
	if (viewportCamera()) {
		viewportCamera()->setResolution(width, height);
	}
	m_deferredShader.setResolution(width, height);
}

void Scene::reloadShaders() {
	ShaderPool::ReloadShaders();
	m_world.reloadShaders();
	m_deferredShader.reloadShaders();

	for (auto obj : m_objects) {
		obj->reloadShaders();
	}
}

void Scene::update(float time) {
	++m_frameIndex;
	if (viewportCamera() && viewportCamera()->outputSettings().isRecordEnabled) {
		// If recording, slow down the game to ensure that it will play back correctly
		time = m_frameIndex / m_fps;
	} else {
		m_frameIndex = static_cast<int>(time * m_fps);
	}

	m_animationManager->update(time, m_frameIndex);

	if (viewportCamera()) {
		viewportCamera()->update(time);
	}

	m_world.update(time);

	for (auto obj : m_objects) {
		obj->update(time, m_frameIndex);
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

		if (s.autoOutputResolution && viewportCamera()->targetFramebuffer()) {
			m_pixels.resize(3 * viewportCamera()->targetFramebuffer()->width() * viewportCamera()->targetFramebuffer()->height());
		}
		else if (s.autoOutputResolution) {
			m_pixels.resize(3 * m_width * m_height);
		}
		else if (m_outputFramebuffer) {
			m_pixels.resize(3 * m_outputFramebuffer->width() * m_outputFramebuffer->height());
		}
	}

	m_mustQuit = m_quitAfterFrame >= 0 && m_frameIndex > m_quitAfterFrame;
}

void Scene::render() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!viewportCamera()) {
		return;
	}
	Camera & camera = *viewportCamera();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_DITHER);

	m_world.renderShadowMaps(camera, m_objects);

	if (m_isDeferredShadingEnabled) {
		m_deferredShader.bindFramebuffer();
	} else if (camera.targetFramebuffer()) {
		camera.targetFramebuffer()->bind();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	const glm::vec2 & res = camera.resolution();
	glViewport(0, 0, static_cast<GLsizei>(res.x), static_cast<GLsizei>(res.y));
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_world.render(camera);
	for (auto obj : m_objects) {
		obj->render(camera, m_world, DefaultRendering);
	}

	if (m_isDeferredShadingEnabled) {
		if (camera.targetFramebuffer()) {
			camera.targetFramebuffer()->bind();
		} else {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		glViewport(0, 0, static_cast<GLsizei>(res.x), static_cast<GLsizei>(res.y));
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_deferredShader.render(camera, m_world, DefaultRendering);
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
		glBlitNamedFramebuffer(camera.targetFramebuffer()->raw(), 0, 0, 0, w1, h1, 0, 0, w2, h2, GL_COLOR_BUFFER_BIT, GL_NEAREST);
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
		obj->onPostRender(time, m_frameIndex);
	}
}

void Scene::clear()
{
	m_objects.clear();
	m_cameras.clear();
	m_world.clear();
	m_animationManager->clear();
	m_frameIndex = -1;
}

std::shared_ptr<Camera> Scene::viewportCamera() const
{
	return m_viewportCameraIndex < m_cameras.size() ? m_cameras[m_viewportCameraIndex] : nullptr;
}

std::shared_ptr<RuntimeObject> Scene::findObjectByName(const std::string& name) {
	for (auto& obj : objects()) {
		if (obj->name == "OcclusionGeometry") {
			return obj;
		}
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
///////////////////////////////////////////////////////////////////////////////

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
	for (int i = 0; i < n; ++i) {
		for (int k = 0; k < p; ++k) {
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
		auto filename = outputSettings.outputFrameBase + number + ".png";

		const void* pixels = static_cast<const void*>(m_pixels.data());
		GLsizei bufSize = static_cast<GLsizei>(m_pixels.size() * sizeof(uint8_t));

		if (outputSettings.autoOutputResolution && camera.targetFramebuffer()) {
			// output camera framebuffer
			camera.targetFramebuffer()->bind();
			GLint destWidth = static_cast<GLint>(camera.targetFramebuffer()->width());
			GLint destHeight = static_cast<GLint>(camera.targetFramebuffer()->height());
			glReadnPixels(0, 0, destWidth, destHeight, GL_RGB, GL_UNSIGNED_BYTE, bufSize, const_cast<void*>(pixels));
			if (outputSettings.saveOnDisc) {
				ResourceManager::saveImage(filename, destWidth, destHeight, pixels);
			}
		}
		else if (outputSettings.autoOutputResolution) {
			// output default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glReadnPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, bufSize, const_cast<void*>(pixels));
			if (outputSettings.saveOnDisc) {
				ResourceManager::saveImage(filename, m_width, m_height, pixels);
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
				ResourceManager::saveImage(filename, destWidth, destHeight, pixels);
			}
		}
	}
}
