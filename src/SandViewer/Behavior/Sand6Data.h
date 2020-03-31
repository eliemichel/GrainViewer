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
#include "IPointCloudData.h"

/**
 * Load point cloud data from sand6 simulation.
 *
 *  Well, actually forget about it: sand6 does not do per grain simulation
 * (that'd be a waste of resources) so there is not way of loading individual
 * grains positions. The particles represent clusters of grains.
 */
class Sand6Data : public Behavior, public IPointCloudData {
public:
	// IPointCloudData implementation
	GLsizei pointCount() const override;
	GLsizei frameCount() const override;
	const GlBuffer & data() const override;
	GLuint vao() const override;

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
