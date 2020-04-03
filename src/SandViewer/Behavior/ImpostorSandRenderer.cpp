
#include "ImpostorSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "BehaviorRegistry.h"
#include "SandBehavior.h"
#include "ResourceManager.h"

#include "utils/jsonutils.h"
#include "utils/behaviorutils.h"

bool ImpostorAtlas::deserialize(const rapidjson::Value& json)
{
	jrOption(json, "enableLeanMapping", enableLeanMapping);

#define readAtlas(name) \
	std::string name; \
	if (jrOption(json, #name, name)) { \
		name ## Texture = ResourceManager::loadTextureStack(name); \
	}
	readAtlas(normalAlpha);
	readAtlas(baseColor);
	readAtlas(metallicRoughness);
#undef readAtlas

	if (normalAlphaTexture && enableLeanMapping) {
		leanTextures = Filtering::CreateLeanTexture(*normalAlphaTexture);
	}

	GLuint n = normalAlphaTexture->depth();
	viewCount = static_cast<GLuint>(sqrt(n / 2));

	return true;
}

void ImpostorAtlas::setUniforms(const ShaderProgram& shader, GLint& textureUnit, const std::string& prefix) const
{
	GLint& o = textureUnit;
	shader.setUniform(prefix + "viewCount", viewCount);

	normalAlphaTexture->bind(o);
	shader.setUniform(prefix + "normalAlphaTexture", o++);

	if (leanTextures) {
		leanTextures->lean1.bind(o);
		shader.setUniform(prefix + "lean1Texture", o++);
		leanTextures->lean2.bind(o);
		shader.setUniform(prefix + "lean2Texture", o++);
	}

	if (baseColorTexture) {
		baseColorTexture->bind(o);
		shader.setUniform(prefix + "baseColorTexture", o++);
	}

	if (metallicRoughnessTexture) {
		metallicRoughnessTexture->bind(o);
		shader.setUniform(prefix + "metallicRoughnesTexture", o++);
		shader.setUniform(prefix + "hasMetallicRoughnessMap", true);
	}
	else {
		shader.setUniform(prefix + "hasMetallicRoughnessMap", false);
	}

	shader.setUniform("hasLeanMapping", enableLeanMapping);
}

//-----------------------------------------------------------------------------

bool ImpostorSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);
	ShaderPool::AddShaderVariant(m_shaderName + "_SHADOW_MAP", m_shaderName, "SHADOW_MAP");

	std::string colormap;
	if (jrOption(json, "colormap", colormap)) {
		m_colormapTexture = ResourceManager::loadTexture(colormap);
	}

	autoDeserialize(json, m_properties);

	jrArray(json, "atlases", m_atlases);

	return true;
}

void ImpostorSandRenderer::start()
{
	m_transform = getComponent<TransformBehavior>();
	m_sand = getComponent<SandBehavior>();
	m_pointData = BehaviorRegistry::getPointCloudDataComponent(*this, PointCloudSplitter::RenderModel::Impostor);
	m_shader = ShaderPool::GetShader(m_shaderName);
	m_shadowMapShader = ShaderPool::GetShader(m_shaderName + "_SHADOW_MAP");
}

void ImpostorSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void ImpostorSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	const ShaderProgram& shader = target == RenderType::ShadowMap ? *m_shadowMapShader : *m_shader;

	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	if (auto sand = m_sand.lock()) {
		autoSetUniforms(shader, sand->properties());
	}

	shader.setUniform("uPointCount", static_cast<GLuint>(pointData->pointCount()));
	shader.setUniform("uFrameCount", static_cast<GLuint>(pointData->frameCount()));
	shader.setUniform("uTime", static_cast<GLfloat>(m_time));
	
	GLint o = 0;

	if (m_colormapTexture) {
		m_colormapTexture->bind(o);
		shader.setUniform("uColormapTexture", o++);
	}

	for (size_t k = 0; k < m_atlases.size(); ++k) {
		m_atlases[k].setUniforms(shader, o, MAKE_STR("impostor[" << k << "]."));
	}

	shader.use();
	glBindVertexArray(pointData->vao());
	if (auto ebo = pointData->ebo()) {
		pointData->vbo().bindSsbo(0);
		ebo->bindSsbo(1);
		shader.setUniform("uUsePointElements", true);
	}
	else {
		shader.setUniform("uUsePointElements", false);
	}
	glDrawArrays(GL_POINTS, pointData->pointOffset(), pointData->pointCount());
	glBindVertexArray(0);
}


//-----------------------------------------------------------------------------

glm::mat4 ImpostorSandRenderer::modelMatrix() const
{
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}

