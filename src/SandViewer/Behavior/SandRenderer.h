#pragma once

#include <memory>
#include "Behavior.h"
#include "PointCloud.h"
#include "GlTexture.h"
#include "GlBuffer.h"

class ShaderProgram;
class MeshDataBehavior;
class TransformBehavior;

class SandRenderer : public Behavior {
public:
	void initShader(ShaderProgram & shader);
	void updateShader(ShaderProgram & shader, float time);

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;
	void update(float time) override;
	void render(const Camera & camera, const World & world, RenderType target) const override;
	void reloadShaders() override;

public:
	// Public properties
	struct Properties {
		float grainRadius = 0.007f;
		float grainMeshScale = 0.45f;
		float instanceLimit = 1.05f;
		bool disableImpostors = false;
		bool disableInstances = false;
	};
	Properties & properties() { return m_properties; }
	const Properties & properties() const { return m_properties; }

private:
	glm::mat4 modelMatrix() const;

	bool load(const PointCloud & pointCloud);
	/**
	* @param textureDirectory Directory containing 2*n*n textures
	* corresponding to the views rendered by script make_octaedron.py with
	* parameter n. The number of files in the directory is used, as well as
	* their alphabetical order.
	*/
	bool loadImpostorTexture(std::vector<std::unique_ptr<GlTexture>> & textures, const std::string & textureDirectory);
	bool loadColormapTexture(const std::string & filename);

	void renderDefault(const Camera & camera, const World & world) const;
	void renderCulling(const Camera & camera, const World & world) const;
	void renderImpostorsDefault(const Camera & camera, const World & world) const;
	void renderInstancesDefault(const Camera & camera, const World & world) const;

private:
	struct PointersSsbo {
		GLint nextInstanceElement;
		GLint nextImpostorElement;
		GLint _pad0[2];
	};

private:
	Properties m_properties;

	std::string m_shaderName = "ImpostorCloud";
	std::string m_shadowMapShaderName;
	std::string m_cullingShaderName = "ImpostorCloudCulling";
	std::string m_instanceCloudShaderName = "InstanceCloud";

	std::shared_ptr<ShaderProgram> m_shader;
	std::shared_ptr<ShaderProgram> m_shadowMapShader;
	std::shared_ptr<ShaderProgram> m_cullingShader;
	std::shared_ptr<ShaderProgram> m_instanceCloudShader;
	bool m_isDeferredRendered;
	size_t m_nbPoints;
	size_t m_frameCount;

	GLuint m_vao;
	GLuint m_vbo;
	std::unique_ptr<GlBuffer> m_commandBuffer;
	std::unique_ptr<GlBuffer> m_cullingPointersSsbo;
	std::unique_ptr<GlBuffer> m_elementBuffer;

	std::weak_ptr<TransformBehavior> m_transform;
	std::weak_ptr<MeshDataBehavior> m_grainMeshData;

	std::vector<std::unique_ptr<GlTexture>> m_normalAlphaTextures;
	std::vector<std::unique_ptr<GlTexture>> m_baseColorTextures;
	std::vector<std::unique_ptr<GlTexture>> m_metallicRoughnessTextures;
	std::unique_ptr<GlTexture> m_colormapTexture;
};

registerBehaviorType(SandRenderer)
