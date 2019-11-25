#pragma once

#include <memory>
#include "Behavior.h"
#include "PointCloud.h"
#include "GlTexture.h"
#include "GlBuffer.h"

class ShaderProgram;

class ImpostorCloudRenderer : public Behavior {
public:
	void renderWithShader(const Camera & camera, const World & world, const ShaderProgram & shader) const;
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

private:
	bool load(const PointCloud & pointCloud);
	/**
	* @param textureDirectory Directory containing 2*n*n textures
	* corresponding to the views rendered by script make_octaedron.py with
	* parameter n. The number of files in the directory is used, as well as
	* their alphabetical order.
	*/
	bool loadImpostorTexture(std::vector<std::unique_ptr<GlTexture>> & textures, const std::string & textureDirectory);
	bool loadColormapTexture(const std::string & filename);

private:
	glm::mat4 m_modelMatrix; // TODO: move to Transform component
	std::string m_shaderName = "point-cloud-multi-view-impostors";
	std::string m_shadowMapShaderName;

	std::shared_ptr<ShaderProgram> m_shader;
	std::shared_ptr<ShaderProgram> m_shadowMapShader;
	bool m_isDeferredRendered;
	size_t m_nbPoints;
	size_t m_frameCount;

	GLuint m_vao;
	GLuint m_vbo;
	std::unique_ptr<GlBuffer> m_drawIndirectBuffer;

	std::vector<std::unique_ptr<GlTexture>> m_normalTextures;
	std::vector<std::unique_ptr<GlTexture>> m_depthTextures;
	std::vector<std::unique_ptr<GlTexture>> m_albedoTextures;
	std::unique_ptr<GlTexture> m_colormapTexture;

	std::vector<std::unique_ptr<ShaderProgram>> m_computeShaders;
	std::unique_ptr<ShaderProgram> m_resetListsComputeShader;
	GLuint m_physicsParticlesBuffer;
	GLuint m_physicsTreeHeadPointerImage;
	GLuint m_physicsTreeNodeBuffer;
};

registerBehaviorType(ImpostorCloudRenderer)
