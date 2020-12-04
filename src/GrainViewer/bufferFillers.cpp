/**
 * This file is part of GrainViewer, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2020).
 *   Real Time Multiscale Rendering of Dense Dynamic Stackings,
 *   Computer Graphics Forum (Proc. Pacific Graphics 2020), 39: 169-179.
 *   https://doi.org/10.1111/cgf.14135
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

#include "bufferFillers.h"

#include <glm/gtc/type_ptr.hpp>


void BufferFiller::fill(PointAttributes* attributes, size_t nbElements) const
{
	BufferFiller::Fill(attributes, nbElements, m_mesh, m_globalOffset);
}

void BufferFiller::Fill(PointAttributes *attributes, size_t nbElements, const Mesh & scene, glm::vec3 globalOffset)
{
	const std::vector<float> & v = scene.attrib().vertices;
	const std::vector<float> & n = scene.attrib().normals;
	const std::vector<float> & uv = scene.attrib().texcoords;

	size_t offset, shapeOffset = 0;
	int vi, ni, uvi;
	for (auto s : scene.shapes()) {
		auto indices = s.mesh.indices;
		for (size_t i = 0; i < indices.size(); ++i) {
			offset = shapeOffset + i;
			vi = indices[i].vertex_index;
			attributes[offset].position[0] = static_cast<GLfloat>(v[3 * vi + 0]) + globalOffset[0];
			attributes[offset].position[1] = static_cast<GLfloat>(v[3 * vi + 1]) + globalOffset[1];
			attributes[offset].position[2] = static_cast<GLfloat>(v[3 * vi + 2]) + globalOffset[2];

			ni = indices[i].normal_index;
			if (ni >= 0) {
				attributes[offset].normal[0] = static_cast<GLfloat>(n[3 * ni + 0]);
				attributes[offset].normal[1] = static_cast<GLfloat>(n[3 * ni + 1]);
				attributes[offset].normal[2] = static_cast<GLfloat>(n[3 * ni + 2]);
			}

			uvi = indices[i].texcoord_index;
			if (uvi >= 0) {
				attributes[offset].texcoords[0] = static_cast<GLfloat>(uv[2 * uvi + 0]);
				attributes[offset].texcoords[1] = static_cast<GLfloat>(uv[2 * uvi + 1]);
			}

			attributes[offset].materialId = static_cast<GLuint>(s.mesh.material_ids[i / 3]);

			if (i % 3 == 2) {
				// End of a triangle: we compute tangent space
				glm::vec3 p0 = glm::make_vec3(attributes[offset - 2].position);
				glm::vec3 p1 = glm::make_vec3(attributes[offset - 1].position);
				glm::vec3 p2 = glm::make_vec3(attributes[offset - 0].position);
				glm::vec2 uv0 = glm::make_vec2(attributes[offset - 2].texcoords);
				glm::vec2 uv1 = glm::make_vec2(attributes[offset - 1].texcoords);
				glm::vec2 uv2 = glm::make_vec2(attributes[offset - 0].texcoords);
				glm::vec3 e1 = p1 - p0;
				glm::vec3 e2 = p2 - p0;
				glm::vec2 delta1 = uv1 - uv0;
				glm::vec2 delta2 = uv2 - uv0;
				glm::vec3 tangent = glm::normalize(e1 * delta2.y - e2 * delta1.y);

				for (size_t j = 0; j < 3; ++j) {
					attributes[offset - j].tangent[0] = static_cast<GLfloat>(tangent.x);
					attributes[offset - j].tangent[1] = static_cast<GLfloat>(tangent.y);
					attributes[offset - j].tangent[2] = static_cast<GLfloat>(tangent.z);
				}
			}
		}

		shapeOffset += indices.size();
	}
}
