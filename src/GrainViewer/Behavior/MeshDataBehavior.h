/**
 * This file is part of GrainViewer
 *
 * Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided “as is”, without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */

#pragma once

#include <OpenGL>

#include "Behavior.h"
#include "Mesh.h"
#include "GlBuffer.h"
#include "StandardMaterial.h"

#include <glm/glm.hpp>

#include <memory>

/**
 * Load mesh from OBJ file to video memory
 */
class MeshDataBehavior : public Behavior {
public:
	// Accessors
	GLsizei pointCount() const;
	GLuint vao() const;
	const std::vector<StandardMaterial>& materials() const { return m_materials; }

	// Available only if m_computeBoundingSphere was true upon start
	bool hasBoundingSphere() const { return m_computeBoundingSphere;  }
	const glm::vec3 & boundingSphereCenter() const { return m_boundingSphereCenter; }
	float boundingSphereRadius() const { return m_boundingSphereRadius; }

	// Must be called *before* start()
	void setFilename(const std::string & filename) { m_filename = filename; }
	const std::string& filename() const { return m_filename; }

public:
	// Behavior implementation
	bool deserialize(const rapidjson::Value & json) override;
	void start() override;
	void onDestroy() override;

private:
	// this take into account a potential TransformBehavior attached to the object
	void computeBoundingSphere();

private:
	std::string m_filename = "";
	std::unique_ptr<Mesh> m_mesh;

	bool m_computeBoundingSphere = false;
	glm::vec3 m_boundingSphereCenter;
	float m_boundingSphereRadius;
	glm::vec3 m_offset = glm::vec3(0);

	GLsizei m_pointCount;
	std::unique_ptr<GlBuffer> m_vertexBuffer;
	GLuint m_vao;
	std::vector<StandardMaterial> m_materials;
};

registerBehaviorType(MeshDataBehavior)

