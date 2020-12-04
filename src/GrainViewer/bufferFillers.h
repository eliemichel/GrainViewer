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

#include <OpenGL>

#include "Mesh.h"

struct PointAttributes {
	GLfloat position[3];
	GLfloat normal[3];
	GLfloat texcoords[2];
	GLuint  materialId;
	GLfloat tangent[3];
};

struct DrawArraysIndirectCommand  {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  first;
	GLuint  baseInstance;
};

struct DrawElementsIndirectCommand {
	GLuint  count;
	GLuint  instanceCount;
	GLuint  firstIndex;
	GLuint  baseVertex;
	GLuint  baseInstance;
};

/**
 * Helper class to fill attribute buffers from a mesh object.
 */
class BufferFiller {
public:
	BufferFiller(const Mesh& mesh) : m_mesh(mesh) {}
	void setGlobalOffset(const glm::vec3& offset) { m_globalOffset = offset; }

	void fill(PointAttributes* attributes, size_t nbElements) const;

public:
	static void Fill(
		PointAttributes* attributes,
		size_t nbElements,
		const Mesh& scene,
		glm::vec3 globalOffset
	);

private:
	const Mesh & m_mesh;
	glm::vec3 m_globalOffset;
};
