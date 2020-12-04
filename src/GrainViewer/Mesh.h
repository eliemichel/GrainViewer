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
	Mesh(const std::string & filename);
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
