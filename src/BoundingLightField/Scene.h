// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

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
	
	void reloadShaders();
	void update(float time);
	void render() const;

	inline Camera & viewportCamera() { return m_viewportCamera; }

	inline const std::vector<std::shared_ptr<RuntimeObject>> & objects() const { return m_objects; }

private:
	std::string m_filename;
	TurntableCamera m_viewportCamera;
	World m_world;
	GlDeferredShader m_deferredShader;
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
	bool m_isDeferredShadingEnabled = true;
};

