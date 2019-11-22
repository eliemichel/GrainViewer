#include "Logger.h"

#include "ResourceManager.h"
#include "ShaderProgram.h"
#include "MeshDataBehavior.h"
#include "MeshRenderer.h"

bool MeshRenderer::deserialize(const rapidjson::Value& json)
{
	if (json.HasMember("shader")) {
		if (json["shader"].IsString()) {
			m_shaderName = json["shader"].GetString();
		}
		else {
			ERR_LOG << "Field 'shader' of MeshRenderer must be a string";
			return false;
		}
	}

	return true;
}

void MeshRenderer::start()
{
	m_shader = std::make_unique<ShaderProgram>(m_shaderName);
	m_meshData = getComponent<MeshDataBehavior>();
}

void MeshRenderer::render(const Camera& camera, const World& world, RenderType target) const
{

}
