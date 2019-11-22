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

class RuntimeObject;

class Scene {
public:
	Scene();

	bool load(const std::string & filename);
	
	void reloadShaders();
	void update(float time);
	void render() const;

	inline Camera & viewport() { return m_camera; }

	inline const std::vector<std::shared_ptr<RuntimeObject>> & objects() const { return m_objects; }

private:
	TurntableCamera m_camera;
	World m_world;
	std::vector<std::shared_ptr<RuntimeObject>> m_objects;
};

