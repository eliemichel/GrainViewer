// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#include <iostream>
#include <tiny_obj_loader.h>

#include "Mesh.h"
#include "utils/fileutils.h"
#include "Logger.h"

using namespace std;

Mesh::Mesh(string filename) {
	m_baseDir = ::baseDir(filename);

    string warn, err;
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


vector<RichTexturedTriangle> Mesh::triangles() const {
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

vector<string> Mesh::diffuseTextures() const {
	vector<string> textures;
	for (auto mat : m_materials) {
		textures.push_back(joinPath(m_baseDir, mat.diffuse_texname));
	}
	return textures;
}

vector<string> Mesh::bumpTextures() const {
	vector<string> textures;
	for (auto mat : m_materials) {
		textures.push_back(joinPath(m_baseDir, mat.bump_texname));
	}
	return textures;
}


#if 0
static void debugMaterial(const tinyobj::material_t & mat) {
	DEBUG_LOG << "Material: " << mat.name;

	DEBUG_LOG << "ambient: " << mat.ambient[0] << ", " << mat.ambient[1] << ", " << mat.ambient[2];
	DEBUG_LOG << "diffuse: " << mat.diffuse[0] << ", " << mat.diffuse[1] << ", " << mat.diffuse[2];
	DEBUG_LOG << "specular: " << mat.specular[0] << ", " << mat.specular[1] << ", " << mat.specular[2];
	DEBUG_LOG << "transmittance: " << mat.transmittance[0] << ", " << mat.transmittance[1] << ", " << mat.transmittance[2];
	DEBUG_LOG << "emission: " << mat.emission[0] << ", " << mat.emission[1] << ", " << mat.emission[2];
	DEBUG_LOG << "shininess: " << mat.shininess;
	DEBUG_LOG << "ior: " << mat.ior;       // index of refraction
	DEBUG_LOG << "dissolve: " << mat.dissolve;  // 1 == opaque; 0 == fully transparent
												// illumination model (see http://www.fileformat.info/format/material/)
	DEBUG_LOG << "illum: " << mat.illum;

	DEBUG_LOG << "dummy: " << mat.dummy;  // Suppress padding warning.

	DEBUG_LOG << "ambient_texname: " << mat.ambient_texname;             // map_Ka
	DEBUG_LOG << "diffuse_texname: " << mat.diffuse_texname;             // map_Kd
	DEBUG_LOG << "specular_texname: " << mat.specular_texname;            // map_Ks
	DEBUG_LOG << "specular_highlight_texname: " << mat.specular_highlight_texname;  // map_Ns
	DEBUG_LOG << "bump_texname: " << mat.bump_texname;                // map_bump, bump
	DEBUG_LOG << "displacement_texname: " << mat.displacement_texname;        // disp
	DEBUG_LOG << "alpha_texname: " << mat.alpha_texname;               // map_d

																	   // PBR extension
																	   // http://exocortex.com/blog/extending_wavefront_mtl_to_support_pbr
	DEBUG_LOG << "roughness: " << mat.roughness;                // [0, 1] default 0
	DEBUG_LOG << "metallic: " << mat.metallic;                 // [0, 1] default 0
	DEBUG_LOG << "sheen: " << mat.sheen;                    // [0, 1] default 0
	DEBUG_LOG << "clearcoat_thickness: " << mat.clearcoat_thickness;      // [0, 1] default 0
	DEBUG_LOG << "clearcoat_roughness: " << mat.clearcoat_roughness;      // [0, 1] default 0
	DEBUG_LOG << "anisotropy: " << mat.anisotropy;               // aniso. [0, 1] default 0
	DEBUG_LOG << "anisotropy_rotation: " << mat.anisotropy_rotation;      // anisor. [0, 1] default 0
	DEBUG_LOG << "roughness_texname: " << mat.roughness_texname;  // map_Pr
	DEBUG_LOG << "metallic_texname: " << mat.metallic_texname;   // map_Pm
	DEBUG_LOG << "sheen_texname: " << mat.sheen_texname;      // map_Ps
	DEBUG_LOG << "emissive_texname: " << mat.emissive_texname;   // map_Ke
	DEBUG_LOG << "normal_texname: " << mat.normal_texname;     // norm. For normal mapping.
}
#endif
