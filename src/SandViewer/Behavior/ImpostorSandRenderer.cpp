
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

	jrOption(json, "metallic", metallic, metallic);
	jrOption(json, "roughness", roughness, roughness);

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
	shader.setUniform(prefix + "metallic", metallic);
	shader.setUniform(prefix + "roughness", roughness);

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
	"PRECOMPUTE_IMPOSTOR_VIEW_MATRICES",
	"PRECOMPUTE_IN_VERTEX",
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
	if (properties().precomputeViewMatrices && !m_precomputedViewMatrices) {
		precomputeViewMatrices();
	}
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
		if (props.precomputeViewMatrices) flags |= ShaderOptionPrecomputeViewMatrices;
		if (props.precomputeInVertex) flags |= ShaderOptionPrecomputeInVertex;
		if (props.interpolationMode == InterpolationMode::None) flags |= ShaderOptionNoInterpolation;
		const ShaderProgram& shader = *getShader(flags);

		if (props.prerenderSurface) {
			setCommonUniforms(shader, camera);
			shader.setUniform("uPrerenderSurfaceStep", 0);
			draw(*pointData, shader);
		}

		setCommonUniforms(shader, camera);
		shader.setUniform("uPrerenderSurfaceStep", 1);
		draw(*pointData, shader);
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
		if (props.precomputeViewMatrices) flags |= ShaderOptionPrecomputeViewMatrices;
		if (props.precomputeInVertex) flags |= ShaderOptionPrecomputeInVertex;
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

void ImpostorSandRenderer::draw(const IPointCloudData& pointData, const ShaderProgram& shader) const
{
	// Draw call
	shader.use();
	glBindVertexArray(pointData.vao());
	if (auto ebo = pointData.ebo()) {
		pointData.vbo().bindSsbo(0);
		ebo->bindSsbo(1);
		shader.setUniform("uUsePointElements", true);
	}
	else {
		shader.setUniform("uUsePointElements", false);
	}
	glDrawArrays(GL_POINTS, pointData.pointOffset(), pointData.pointCount());
	glBindVertexArray(0);
}

void ImpostorSandRenderer::setCommonUniforms(const ShaderProgram& shader, const Camera& camera) const
{
	const Properties& props = properties();

	// Set uniforms
	glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
	shader.bindUniformBlock("Camera", camera.ubo());
	shader.setUniform("modelMatrix", modelMatrix());
	shader.setUniform("viewModelMatrix", viewModelMatrix);

	autoSetUniforms(shader, properties());
	if (auto sand = m_sand.lock()) {
		autoSetUniforms(shader, sand->properties());
	}

	auto pointData = m_pointData.lock();
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

	if (props.precomputeViewMatrices) {
		m_precomputedViewMatrices->bindSsbo(4);
	}

	shader.setUniform("uUseOcclusionMap", false);
	if (auto splitter = m_splitter.lock()) {
		if (splitter->properties().enableOcclusionCulling) {
			// This is a hack: we reuse the fbo that was used by the splitter and assume nothing else has written to it in the meantime
			auto occlusionCullingFbo = camera.getExtraFramebuffer(Camera::ExtraFramebufferOption::Rgba32fDepth);
			glBindTextureUnit(static_cast<GLuint>(o), occlusionCullingFbo->colorTexture(0));
			shader.setUniform("uOcclusionMap", o++);
			shader.setUniform("uUseOcclusionMap", true);
		}
	}
}

// Extracted from impostor.inc.glsl
namespace glsl {
	using namespace glm;

	/**
	 * Return the direction of the i-th plane in an octahedric division of the unit
	 * sphere of n subdivisions.
	 */
	vec3 ViewIndexToDirection(uint i, uint n) {
		float eps = -1;
		uint n2 = n * n;
		if (i >= n2) {
			eps = 1;
			i -= n2;
		}
		vec2 uv = vec2(i / n, i % n) / float(n - 1);
		float x = uv.x + uv.y - 1;
		float y = uv.x - uv.y;
		float z = eps * (1.0f - abs(x) - abs(y));
		// break symmetry in redundant parts. TODO: find a mapping without redundancy, e.g. full octahedron
		if (z == 0) z = 0.0001f * eps;
		return normalize(vec3(x, y, z));
	}

	/**
	 * Build the inverse of the view matrix used to bake the i-th G-Impostor
	 */
	mat4 InverseBakingViewMatrix(uint i, uint n) {
		vec3 z = ViewIndexToDirection(i, n);
		vec3 x = normalize(cross(vec3(0, 0, 1), z));
		vec3 y = normalize(cross(z, x));
		vec3 w = vec3(0);
		return transpose(mat4(
			vec4(x, 0),
			vec4(y, 0),
			vec4(z, 0),
			vec4(w, 1)
			));
	}
}

void ImpostorSandRenderer::precomputeViewMatrices()
{
	m_precomputedViewMatrices = std::make_unique<GlBuffer>(GL_SHADER_STORAGE_BUFFER);
	// Check that viewCount is the same for all impostors
	GLuint n = m_atlases[0].viewCount;
	for (int i = 1 ; i < m_atlases.size() ; ++i) {
		if (m_atlases[i].viewCount != n) {
			ERR_LOG << "Precomputed view matrices can only be used when all impostors use the same number of views";
			properties().precomputeViewMatrices = false;
			return;
		}
	}
	m_precomputedViewMatrices->addBlock<glm::mat4>(static_cast<size_t>(2 * n * n));
	m_precomputedViewMatrices->alloc();
	m_precomputedViewMatrices->fillBlock<glm::mat4>(0, [n](glm::mat4 *data, size_t size) {
		for (glm::uint i = 0; i < size; ++i) {
			data[i] = glsl::InverseBakingViewMatrix(i, n);
		}
	});
	m_precomputedViewMatrices->finalize();
}

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

