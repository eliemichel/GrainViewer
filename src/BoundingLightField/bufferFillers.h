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

void fillPointAttributes(
	PointAttributes *attributes,
	size_t nbElements,
	const Mesh & scene
);
