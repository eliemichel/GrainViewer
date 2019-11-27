// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include "Triangle.h"
#include <tiny_obj_loader.h>
#include <string>
#include <vector>

/**
 * Simple wrapper around TinyObj loading routines
 */
class Mesh {
public:
	Mesh(std::string filename);
    std::vector<RichTexturedTriangle> triangles() const;
	std::vector<std::string> diffuseTextures() const;
	std::vector<std::string> bumpTextures() const;

	inline tinyobj::attrib_t attrib() const { return m_attrib; }
	inline std::vector<tinyobj::shape_t> shapes() const { return m_shapes; }
	inline std::vector<tinyobj::material_t> materials() const { return m_materials; }

	inline const std::string baseDir() const { return m_baseDir; }

private:
	std::string m_baseDir;
    tinyobj::attrib_t m_attrib;
    std::vector<tinyobj::shape_t> m_shapes;
    std::vector<tinyobj::material_t> m_materials;
};
