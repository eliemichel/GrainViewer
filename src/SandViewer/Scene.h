// **************************************************
// Author : �lie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 �lie Michel.
// **************************************************

#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include "Camera.h"
#include "TurntableCamera.h"
#include "World.h"
#include "GlDeferredShader.h"

class AnimationManager;
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
	void onPostRender(float time);

	// Clear scene
	void clear();

	std::shared_ptr<Camera> viewportCamera() const;
	inline const std::vector<std::shared_ptr<RuntimeObject>> & objects() const { return m_objects; }

	World & world() { return m_world; }
	GlDeferredShader & deferredShader() { return m_deferredShader; }

	bool mustQuit() const { return m_mustQuit;  }
	int frame() const { return m_frameIndex; }

	std::shared_ptr<RuntimeObject> findObjectByName(const std::string& name);

private:
	void recordFrame(const Camera & camera) const;
	void measureStats();

private:
	std::string m_filename;
	World m_world;
	GlDeferredShader m_deferredShader;
	int m_viewportCameraIndex;
	std::vector<std::shared_ptr<Camera>> m_cameras;
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	std::shared_ptr<AnimationManager> m_animationManager;
	bool m_isDeferredShadingEnabled = true;

	// Framebuffer used before writing image if the output resolution is different from camera resolution
	std::unique_ptr<Framebuffer> m_outputFramebuffer;
	int m_frameIndex = -1;
	std::vector<uint8_t> m_pixels;

	float m_fps;
	int m_quitAfterFrame = -1; // -1 to deactivate this feature, otherwise automatically quit the program after the specified frame (usefull for batch rendering)
	bool m_mustQuit = false;

	// Post render stats
	std::string m_outputStats; // path to stats file
	std::vector<glm::vec3> m_statsCountColors; // colors to count pixels in render
	std::ofstream m_outputStatsFile;

	// Not really related to the scene, save window resolution
	int m_width;
	int m_height;
};

