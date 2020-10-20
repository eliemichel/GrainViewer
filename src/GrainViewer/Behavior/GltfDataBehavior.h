#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include <memory>
#include "tiny_gltf.h"

#include "Behavior.h"

class ShaderProgram;
class TransformBehavior;

/**
 * Load mesh from gltf file using tinygltf
 */
class GltfDataBehavior : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;
	void render(const Camera& camera, const World& world, RenderType target) const override;

private:
	glm::mat4 modelMatrix() const;

private:
	struct DrawCall {
		GLenum mode;
		GLsizei count;
		GLenum type;
		size_t byteOffset;
	};

	std::string m_filename;
	std::string m_shaderName = "GltfMesh";
	std::shared_ptr<ShaderProgram> m_shader;
	std::unique_ptr<tinygltf::Model> m_model;
	std::vector<GLuint> m_buffers;
	std::vector<DrawCall> m_drawCalls;
	std::vector<GLuint> m_vertexArrays;

	std::weak_ptr<TransformBehavior> m_transform;
};

registerBehaviorType(GltfDataBehavior)

