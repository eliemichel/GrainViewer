// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

#include <glad/glad.h>
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

void fillPointAttributes(
	PointAttributes *attributes,
	size_t nbElements,
	const Mesh & scene
);
