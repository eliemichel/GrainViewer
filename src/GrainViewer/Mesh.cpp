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

#include "Mesh.h"
#include "utils/fileutils.h"
#include "Logger.h"

#include <tiny_obj_loader.h>

#include <iostream>

Mesh::Mesh(const std::string & filename) {
	m_baseDir = ::baseDir(filename);

    std::string warn, err;
    bool ret = tinyobj::LoadObj(&m_attrib, &m_shapes, &m_materials, &warn, &err, filename.c_str(), joinPath(m_baseDir, "").c_str(), true);
	if (!warn.empty()) {
		WARN_LOG << "TinyObj returned: " << warn;
	}
    if (!err.empty()) {
		ERR_LOG << "TinyObj returned: " << err;
    }
    if (!ret) {
        exit(EXIT_FAILURE);
    }

    DEBUG_LOG << "# of shapes    : " << m_shapes.size();
	DEBUG_LOG << "# of materials : " << m_materials.size();
}


std::vector<RichTexturedTriangle> Mesh::triangles() const {
	RichTexturedTriangle tri;
    std::vector<RichTexturedTriangle> triangles;

    const std::vector<float> & v = m_attrib.vertices;
    const std::vector<float> & n = m_attrib.normals;
	const std::vector<float> & uv = m_attrib.texcoords;
    int f0, f1, f2;
    for (size_t s = 0; s < m_shapes.size(); ++s) {
        const tinyobj::mesh_t & mesh = m_shapes[s].mesh;
        for (size_t f = 0; f < mesh.indices.size() / 3; ++f) {
			// Positions
            f0 = mesh.indices[3 * f + 0].vertex_index;
            f1 = mesh.indices[3 * f + 1].vertex_index;
            f2 = mesh.indices[3 * f + 2].vertex_index;
            tri.a = glm::vec3(v[3 * f0], v[3 * f0 + 1], v[3 * f0 + 2]);
            tri.b = glm::vec3(v[3 * f1], v[3 * f1 + 1], v[3 * f1 + 2]);
            tri.c = glm::vec3(v[3 * f2], v[3 * f2 + 1], v[3 * f2 + 2]);

			// Normals
            f0 = mesh.indices[3 * f + 0].normal_index;
            f1 = mesh.indices[3 * f + 1].normal_index;
            f2 = mesh.indices[3 * f + 2].normal_index;
            if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                tri.na = glm::vec3(n[3 * f0], n[3 * f0 + 1], n[3 * f0 + 2]);
                tri.nb = glm::vec3(n[3 * f1], n[3 * f1 + 1], n[3 * f1 + 2]);
                tri.nc = glm::vec3(n[3 * f2], n[3 * f2 + 1], n[3 * f2 + 2]);
            }
            else {
                // flat normal
                vec3 u = tri.b - tri.a;
                vec3 v = tri.c - tri.a;
                vec3 n = normalize(cross(u, v));
                tri.na = tri.nb = tri.nc = n;
            }

			// UV coords
			f0 = mesh.indices[3 * f + 0].texcoord_index;
			f1 = mesh.indices[3 * f + 1].texcoord_index;
			f2 = mesh.indices[3 * f + 2].texcoord_index;
			if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
				tri.uva = glm::vec2(uv[2 * f0], uv[2 * f0 + 1]);
				tri.uvb = glm::vec2(uv[2 * f1], uv[2 * f1 + 1]);
				tri.uvc = glm::vec2(uv[2 * f2], uv[2 * f2 + 1]);
			}
			else {
				tri.uva = tri.uvb = tri.uvc = glm::vec2(0.f);
			}

            tri.materialId = mesh.material_ids[f];
            if (tri.materialId < 0 || tri.materialId >= m_materials.size()) {
                tri.materialId = 0;  // default material
            }
            triangles.push_back(tri);
        }
    }

    return triangles;
}

std::vector<std::string> Mesh::diffuseTextures() const {
	std::vector<std::string> textures;
	for (auto mat : m_materials) {
		textures.push_back(joinPath(m_baseDir, mat.diffuse_texname));
	}
	return textures;
}

std::vector<std::string> Mesh::bumpTextures() const {
	std::vector<std::string> textures;
	for (auto mat : m_materials) {
		textures.push_back(joinPath(m_baseDir, mat.bump_texname));
	}
	return textures;
}
