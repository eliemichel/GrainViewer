#pragma once
#ifndef NO_GPL

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/glad.h>
#include <memory>
#include <glm/glm.hpp>

#include <refl.hpp>
#include <visu/Offline.hh> // sand6

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"

/**
 * Load point cloud data from sand6 simulation
 */
class Sand6Data : public Behavior {
public:
	// Accessors
	GLsizei pointCount() const;
	GLsizei frameCount() const;
	const GlBuffer & data() const;
	GLuint vao() const;

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value& json) override;
	void start() override;
	void onDestroy() override;

public:
	// proerties
	struct Properties {
		bool animated = true;
	};
	const Properties & properties() const { return m_properties; }
	Properties & properties(){ return m_properties; }

private:
	Properties m_properties;

	std::string m_directory = "";

	GLsizei m_pointCount;
	GLsizei m_frameCount;
	std::unique_ptr<GlBuffer> m_pointBuffer;
	GLuint m_vao;
};

REFL_TYPE(Sand6Data::Properties)
REFL_FIELD(animated)
REFL_END

registerBehaviorType(Sand6Data)

#endif // ndef NO_GPL
