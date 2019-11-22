#include "Logger.h"

#include "ResourceManager.h"
#include "MeshDataBehavior.h"

bool MeshDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename")) {
		if (json["filename"].IsString()) {
			m_filename = json["filename"].GetString();
		} else {
			ERR_LOG << "Field 'filename' of MeshDataBehavior must be a string";
			return false;
		}
	}

	if (m_filename == "") {
		ERR_LOG << "MeshDataBehavior requires a filename to load";
		return false;
	}

	m_filename = ResourceManager::resolveResourcePath(m_filename);

	return true;
}

void MeshDataBehavior::start()
{
	m_mesh = std::make_unique<Mesh>(m_filename);
}
