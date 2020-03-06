#include "Logger.h"
#include "utils/strutils.h"
#include "ResourceManager.h"
#include "GltfDataBehavior.h"

///////////////////////////////////////////////////////////////////////////////
// Accessors
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Behavior Implementation
///////////////////////////////////////////////////////////////////////////////

bool GltfDataBehavior::deserialize(const rapidjson::Value & json)
{
	if (json.HasMember("filename") && json["filename"].IsString()) {
		m_filename = ResourceManager::resolveResourcePath(json["filename"].GetString());
		return true;
	} else {
		ERR_LOG << "GltfDataBehavior requires a filename to load";
		return false;
	}
}

void GltfDataBehavior::start()
{
	// 1. Load GLTF
	m_model = std::make_unique<tinygltf::Model>();
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret = false;
	if (endsWith(m_filename, ".glb")) {
		ret = loader.LoadBinaryFromFile(m_model.get(), &err, &warn, m_filename.c_str());
	} else {
		ret = loader.LoadASCIIFromFile(m_model.get(), &err, &warn, m_filename.c_str());
	}
	if (!err.empty()) {
		ERR_LOG << "tinygltf: " << err;
	}
	if (!warn.empty()) {
		WARN_LOG << "tinygltf: " << warn;
	}
	if (!ret) {
		ERR_LOG << "Unable to open GLTF file: " << m_filename << " (see error above)";
		return;
	}

	// 2. Move data to GlBuffer (in VRAM)
	// Build VBO
	// Build VAO

	// 3. Free mesh from RAM now that it is in VRAM
}

void GltfDataBehavior::onDestroy()
{
	
}
