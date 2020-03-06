#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <memory>
#include "tiny_gltf.h"

#include "Behavior.h"

/**
 * Load mesh from gltf file using tinygltf
 */
class GltfDataBehavior : public Behavior {
public:
	// Accessors
	GLsizei pointCount() const;
	GLuint vao() const;

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	std::string m_filename;
	std::unique_ptr<tinygltf::Model> m_model;
};

registerBehaviorType(GltfDataBehavior)

