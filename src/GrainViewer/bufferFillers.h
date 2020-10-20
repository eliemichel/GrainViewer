// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

#include "Mesh.h"

typedef struct {
	GLfloat position[3];
	GLfloat normal[3];
	GLfloat texcoords[2];
	GLuint  materialId;
	GLfloat tangent[3];
} PointAttributes;

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

// for backward compat
void fillPointAttributes(
	PointAttributes* attributes,
	size_t nbElements,
	const Mesh& scene
	);
