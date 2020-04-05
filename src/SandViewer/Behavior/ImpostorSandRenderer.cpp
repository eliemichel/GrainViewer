
#include "ImpostorSandRenderer.h"
#include "TransformBehavior.h"
#include "ShaderPool.h"
#include "BehaviorRegistry.h"
#include "SandBehavior.h"
#include "ResourceManager.h"
#include "utils/ScopedFramebufferOverride.h"

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

const std::vector<std::string> ImpostorSandRenderer::s_shaderVariantDefines = {
	"NO_DISCARD",
	"PASS_SHADOW_MAP",
	"PASS_BLIT_TO_MAIN_FBO",
	"NO_INTERPOLATION",
};

bool ImpostorSandRenderer::deserialize(const rapidjson::Value & json)
{
	jrOption(json, "shader", m_shaderName, m_shaderName);

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
	m_splitter = getComponent<PointCloudSplitter>();
}

void ImpostorSandRenderer::update(float time, int frame)
{
	m_time = time;
}

void ImpostorSandRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	auto pointData = m_pointData.lock();
	if (!pointData) return;

	ScopedFramebufferOverride scoppedFramebufferOverride; // to automatically restore fbo binding at the end of scope
	const Properties& props = properties();

	glEnable(GL_PROGRAM_POINT_SIZE);

	std::shared_ptr<Framebuffer> fbo;
	if (props.noDiscard && target != RenderType::ShadowMap) {
		// If not using discards in main draw call, we render in a separate
		// framebuffer and only then blit it onto the main framebuffer
		fbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::LinearGBufferDepth);
		fbo->bind();
	}

	// 1. Clear depth
	if (fbo) {
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// 2. Main drawing, cumulativly if there is an extra fbo
	{
		glDepthMask(GL_TRUE);
		if (fbo) {
			//glDisable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);

			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		else {
			glEnable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
		}

		// Get shader
		ShaderVariantFlagSet flags = 0;
		if (target == RenderType::ShadowMap) flags |= ShaderPassShadow;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.interpolationMode == InterpolationMode::None) flags |= ShaderOptionNoInterpolation;
		const ShaderProgram& shader = *getShader(flags);

		// Set uniforms
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

		if (auto splitter = m_splitter.lock()) {
			// This is a hack: we reuse the fbo that was used by the splitter and assume nothing else has written to it in the meantime
			auto occlusionCullingFbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::Rgba32fDepth);
			//occlusionCullingFbo->colorTexture(0);
		}

		// Draw call
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

	// 3. Blit auxilliary fbo to main fbo
	if (fbo) {
		scoppedFramebufferOverride.restore();

		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		// Get shader
		ShaderVariantFlagSet flags = ShaderPassBlitToMainFbo;
		if (target == RenderType::ShadowMap) flags |= ShaderPassShadow;
		if (props.noDiscard) flags |= ShaderOptionNoDiscard;
		if (props.interpolationMode == InterpolationMode::None) flags |= ShaderOptionNoInterpolation;
		const ShaderProgram& shader = *getShader(flags);

		// Set uniforms

		// Bind secondary FBO textures
		glTextureBarrier();
		GLint o = 0;
		for (int i = 0; i < fbo->colorTextureCount(); ++i) {
			glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
			shader.setUniform(MAKE_STR("lgbuffer" << i), o);
			++o;
		}
		glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
		shader.setUniform("uFboDepthTexture", o);
		++o;

		shader.use();
		PostEffect::DrawWithDepthTest();
	}
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

std::shared_ptr<ShaderProgram> ImpostorSandRenderer::getShader(ShaderVariantFlagSet flags) const
{
	constexpr int nFlags = static_cast<int>(magic_enum::enum_count<ShaderVariantFlag>());
	if (m_shaders.empty()) {
		m_shaders.resize(1 << nFlags);
	}

	if (!m_shaders[flags]) {
		// Lazy loading of shader variants
		std::string variantName = m_shaderName + "_ShaderVariantFlags_" + bitname(flags, nFlags);
		std::vector<std::string> defines;
		for (int f = 0; f < nFlags; ++f) {
			if ((flags & (1 << f)) != 0) {
				defines.push_back(s_shaderVariantDefines[f]);
			}
		}
		DEBUG_LOG << "loading variant " << variantName;
		ShaderPool::AddShaderVariant(variantName, m_shaderName, defines);
		m_shaders[flags] = ShaderPool::GetShader(variantName);
	}
	return m_shaders[flags];
}

