
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include <refl.hpp>
#include <glm/glm.hpp>
#include <memory>

class ShaderProgram;
class TransformBehavior;

/**
 * Behavior holding sand properties that are common to all sand renderers.
 */
class SandBehavior : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;

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
};

#define _ ReflectionAttributes::
REFL_TYPE(SandBehavior::Properties)
REFL_FIELD(grainRadius, _ Range(0.0f, 0.1f))
REFL_FIELD(grainInnerRadiusRatio, _ Range(0.0f, 1.0f))
REFL_FIELD(debugRenderType)
REFL_END
#undef _

registerBehaviorType(SandBehavior)
