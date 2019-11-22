#pragma once

#include <memory>

#include "Behavior.h"
#include "Mesh.h"

class MeshDataBehavior : public Behavior {
public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;

private:
	std::string m_filename = "";
	std::unique_ptr<Mesh> m_mesh;
};

registerBehaviorType(MeshDataBehavior)

