// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
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
#include "Framebuffer.h"

class World;
class GlDeferredShader;
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

	std::shared_ptr<World> world() { return m_world; }
	std::shared_ptr<GlDeferredShader> deferredShader() { return m_deferredShader; }

	bool mustQuit() const { return m_mustQuit;  }
	int frame() const { return m_frameIndex; }

	std::shared_ptr<RuntimeObject> findObjectByName(const std::string& name);

	void takeScreenshot() const;

private:
	void measureStats();
	// TODO: This should be in another section of the code
	enum RecordFormat {
		RecordExr,
		RecordPng,
	};
	// Return the number of pixels in output images
	size_t getOutputPixelCount(const Camera& camera) const;
	void recordFrame(const Camera & camera, const std::string & filename, RecordFormat format) const;
	// Record frame only if enabled in camera options
	void recordFrame(const Camera & camera) const;

private:
	std::string m_filename;
	std::shared_ptr<World> m_world;
	std::shared_ptr<GlDeferredShader> m_deferredShader;
	int m_viewportCameraIndex;
	std::vector<std::shared_ptr<Camera>> m_cameras;
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	std::shared_ptr<AnimationManager> m_animationManager;
	bool m_isDeferredShadingEnabled = true;

	// Framebuffer used before writing image if the output resolution is different from camera resolution
	mutable std::unique_ptr<Framebuffer> m_outputFramebuffer; // lazyly allocated in recordFrame
	int m_frameIndex = -1;
	// TODO: wrap those data into proper logic/class
	std::vector<uint8_t> m_pixels;
	mutable std::vector<GLfloat> m_floatPixels; // for EXR screenshots

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

