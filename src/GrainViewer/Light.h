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

#include "ShadowMap.h"

#include <glm/glm.hpp>

/**
 * Simple point light with shadow map.
 */
class Light {
public:
	Light(const glm::vec3 & position, const glm::vec3 & color, size_t shadowMapSize = 1024, bool isRich = false, bool hasShadowMap = true);

	inline glm::vec3 position() const { return m_lightPosition; }
	void setPosition(const glm::vec3 & position);

	inline glm::vec3 lookAt() const { return m_lookAt; }

	inline const glm::vec3 & color() const { return m_lightColor; }
	inline glm::vec3 & color() { return m_lightColor; }

	inline bool isRich() const { return m_isRich; }

	inline bool hasShadowMap() const { return m_hasShadowMap; }
	inline void setHasShadowMap(bool value) { m_hasShadowMap = value; }

	inline const ShadowMap & shadowMap() const { return *m_shadowMap; }
	inline ShadowMap & shadowMap() { return *m_shadowMap; }

	virtual void update(float time) {}

protected:
	glm::vec3 m_lightPosition;
	glm::vec3 m_lookAt;
	glm::vec3 m_lightColor;
	std::unique_ptr<ShadowMap> m_shadowMap;
	bool m_isRich;
	bool m_hasShadowMap;
};

/**
 * Slight variant, turning around the origin
 */
class TurningLight : public Light {
public:
	TurningLight(const glm::vec3 & position, const glm::vec3 & color, size_t shadowMapSize = 1024, bool isRich = false, bool hasShadowMap = true);

	void update(float time) override;

protected:
	glm::vec3 m_initialPosition;
};
