#include "Logger.h"

#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "MeshDataBehavior.h"
#include "MeshRenderer.h"
#include "ShaderPool.h"
#include "TransformBehavior.h"
#include "GlTexture.h"
#include "utils/jsonutils.h"

///////////////////////////////////////////////////////////////////////////////
// Material
///////////////////////////////////////////////////////////////////////////////

GLuint MeshRenderer::Material::setUniforms(const ShaderProgram & shader, GLuint id, GLuint firstTextureIndex) const
{
	GLuint o = firstTextureIndex;
	const std::string prefix = "material[" + std::to_string(id) + "]";
	if (baseColorMap) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		baseColorMap->bind();
		shader.setUniform(prefix + ".baseColorMap", static_cast<GLint>(o));
		++o;
		shader.setUniform(prefix + ".hasBaseColorMap", true);
	} else {
		shader.setUniform(prefix + ".hasBaseColorMap", false);
	}
	if (normalMap) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		normalMap->bind();
		shader.setUniform(prefix + ".normalMap", static_cast<GLint>(o));
		++o;
		shader.setUniform(prefix + ".hasNormalMap", true);
	} else {
		shader.setUniform(prefix + ".hasNormalMap", false);
	}
	if (metallicRoughnessMap) {
		glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(o));
		metallicRoughnessMap->bind();
		shader.setUniform(prefix + ".metallicRoughnessMap", static_cast<GLint>(o));
		++o;
		shader.setUniform(prefix + ".hasMetallicRoughnessMap", true);
	} else {
		shader.setUniform(prefix + ".hasMetallicRoughnessMap", false);
	}
	shader.setUniform(prefix + ".baseColor", baseColor);
	shader.setUniform(prefix + ".metallic", metallic);
	shader.setUniform(prefix + ".roughness", roughness);
	return o;
}

// private classes for serialization only
class MaterialModel {
public:
	bool readJson(const rapidjson::Value & json) { JREADp(baseColor); JREADp(metallic); JREADp(roughness); JREADp(baseColorMap); JREADp(normalMap); JREADp(metallicRoughnessMap); return true; }

	glm::vec3 baseColor;
	float metallic;
	float roughness;
	std::string baseColorMap;
	std::string normalMap;
	std::string metallicRoughnessMap;
};
void MeshRenderer::Material::deserializeArray(const rapidjson::Value& json, const std::string & key, std::vector<MeshRenderer::Material> & output)
{
	std::vector<MaterialModel> materialsData;
	jrArray<MaterialModel>(json, key, materialsData);
	for (const auto& m : materialsData) {
		Material mat;
		if (!m.baseColorMap.empty()) {
			mat.baseColorMap = ResourceManager::loadTexture(m.baseColorMap);
		}
		if (!m.normalMap.empty()) {
			mat.normalMap = ResourceManager::loadTexture(m.normalMap);
		}
		if (!m.metallicRoughnessMap.empty()) {
			mat.metallicRoughnessMap = ResourceManager::loadTexture(m.metallicRoughnessMap);
		}
		mat.baseColor = m.baseColor;
		mat.metallic = static_cast<GLfloat>(m.metallic);
		mat.roughness = static_cast<GLfloat>(m.roughness);
		output.push_back(std::move(mat));
	}
}

///////////////////////////////////////////////////////////////////////////////
// Behavior implementation
///////////////////////////////////////////////////////////////////////////////

bool MeshRenderer::deserialize(const rapidjson::Value& json)
{
	if (json.HasMember("shader")) {
		if (json["shader"].IsString()) {
			m_shaderName = json["shader"].GetString();
		} else {
			ERR_LOG << "Field 'shader' of MeshRenderer must be a string";
			return false;
		}
	}

	Material::deserializeArray(json, "materials", m_materials);

	return true;
}

void MeshRenderer::start()
{
	m_shader = ShaderPool::GetShader(m_shaderName);
	if (!m_shader) {
		WARN_LOG << "Using direct shader name in MeshRenderer is depreciated, use 'shaders' section to define shaders.";
		m_shader = std::make_shared<ShaderProgram>(m_shaderName);
	}
	m_meshData = getComponent<MeshDataBehavior>();
	m_transform = getComponent<TransformBehavior>();
}

void MeshRenderer::render(const Camera& camera, const World& world, RenderType target) const
{
	if (!m_shader->isValid()) return;

	if (auto mesh = m_meshData.lock()) {
		m_shader->use();

		glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
		m_shader->bindUniformBlock("Camera", camera.ubo());
		m_shader->setUniform("modelMatrix", modelMatrix());
		m_shader->setUniform("viewModelMatrix", viewModelMatrix);

		GLuint o = 0;
		GLuint matId = 0;
		for (const auto& mat : m_materials) {
			o = mat.setUniforms(*m_shader, matId, o);
			++matId;
		}

		glBindVertexArray(mesh->vao());
		glDrawArrays(GL_TRIANGLES, 0, mesh->pointCount());
		glBindVertexArray(0);
	}
}

///////////////////////////////////////////////////////////////////////////////
// private members
///////////////////////////////////////////////////////////////////////////////

glm::mat4 MeshRenderer::modelMatrix() const {
	if (auto transform = m_transform.lock()) {
		return transform->modelMatrix();
	} else {
		return glm::mat4(1.0f);
	}
}