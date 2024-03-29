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

#pragma once

#include <OpenGL>

#include "Camera.h"

#include <rapidjson/document.h>

#include <vector>
#include <memory>

class Light;
class ShaderProgram;
class RuntimeObject;
class AnimationManager;

/**
 * Contains all lighting information for a render
 */
class World {
public:
	World();
	bool deserialize(const rapidjson::Value & json, std::shared_ptr<AnimationManager> animations);
	void start();
	void update(float time);
	void reloadShaders();
	void onPreRender(const Camera & camera) const;
	void render(const Camera & camera) const;
	void renderShadowMaps(const std::vector<std::shared_ptr<RuntimeObject>> & objects) const;

	const std::vector<std::shared_ptr<Light>> & lights() const { return m_lights; }

	void clear();

	bool isShadowMapEnabled() const { return m_isShadowMapEnabled; }
	void setShadowMapEnabled(bool value) { m_isShadowMapEnabled = value; }

private:
	void initVao();

private:
	std::string m_shaderName = "World";
	std::vector<std::shared_ptr<Light>> m_lights;
	std::shared_ptr<ShaderProgram> m_shader;
	GLuint m_vbo; // TODO: use GlBuffer here!
	GLuint m_vao;
	bool m_isShadowMapEnabled = true;
};
