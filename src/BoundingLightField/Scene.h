// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#if _WIN32
#include <windows.h>
#endif
#include <glad/glad.h>

#include <memory>
#include <vector>
#include <string>

#include "Camera.h"
#include "TurntableCamera.h"
#include "World.h"
#include "GlDeferredShader.h"

class RuntimeObject;

class Scene {
public:
	Scene();

	bool load(const std::string & filename);
	const std::string & filename() const { return m_filename; }

	void setResolution(int width, int height);
	
	void reloadShaders();
	void update(float time);
	void render() const;

	// Clear scene
	void clear();

	std::shared_ptr<Camera> viewportCamera() const;
	inline const std::vector<std::shared_ptr<RuntimeObject>> & objects() const { return m_objects; }

private:
	void recordFrame(const Camera & camera) const;

private:
	std::string m_filename;
	World m_world;
	GlDeferredShader m_deferredShader;
	int m_viewportCameraIndex;
	std::vector<std::shared_ptr<Camera>> m_cameras;
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	bool m_isDeferredShadingEnabled = true;

	// Framebuffer used before writing image if the output resolution is different from camera resolution
	std::unique_ptr<Framebuffer> m_outputFramebuffer;
	int m_frameIndex;
	std::vector<uint8_t> m_pixels;

	// Not really related to the scene, save window resolution
	int m_width;
	int m_height;
};

