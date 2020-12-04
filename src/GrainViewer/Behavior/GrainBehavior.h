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

#include "Behavior.h"
#include "ImpostorAtlasMaterial.h"

#include <refl.hpp>

#include <glm/glm.hpp>

#include <memory>

class ShaderProgram;
class TransformBehavior;

/**
 * Behavior holding sagrainnd properties that are common to all grain renderers.
 */
class GrainBehavior : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	const std::vector<ImpostorAtlasMaterial> & atlases() const { return m_atlases; }

public:
	// Properties (serialized and displayed in UI)
	struct Properties {
		// Outer radius of the grain. The grain is entierly contained in a sphere of this radius.
		float grainRadius = 0.01f;
		// Inner over outer radius ratio. The sphere of inner radius is entierly contained in the grain geometry.
		// This ratio ranges from 0 (arbitrary grain) to 1 (perfectly spherical grain).
		float grainInnerRadiusRatio = 0.8f;
		// If true, fake colors are used to display which model is used to render which grains.
		bool debugRenderType = false;
	};
	Properties & properties() { return m_properties; }
	const Properties& properties() const { return m_properties; }

private:
	Properties m_properties;
	std::vector<ImpostorAtlasMaterial> m_atlases;
};

#define _ ReflectionAttributes::
REFL_TYPE(GrainBehavior::Properties)
REFL_FIELD(grainRadius, _ Range(0.0f, 0.1f))
REFL_FIELD(grainInnerRadiusRatio, _ Range(0.0f, 1.0f))
REFL_FIELD(debugRenderType)
REFL_END
#undef _

registerBehaviorType(GrainBehavior)
