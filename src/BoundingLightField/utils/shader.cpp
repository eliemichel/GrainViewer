/**
* This file is part of Augen Light
*
* Copyright (c) 2017 - 2018 -- Élie Michel <elie.michel@exppad.com>
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

#include "shader.h"

#include <iostream>

bool checkShader(GLuint shader, const char *name) {
	int ok;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		int len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		char *log = new char[len];
		glGetShaderInfoLog(shader, len, &len, log);
		std::cerr
			<< "Unable to compile " << name << ". OpenGL returned:" << std::endl
			<< log << std::endl << std::endl;
		delete[] log;
	}

	return ok;
}

bool checkProgram(GLuint program) {
	int ok;

	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (!ok) {
		int len;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		char *log = new char[len];
		glGetProgramInfoLog(program, len, &len, log);
		std::cerr
			<< "ERROR: Unable to link program. OpenGL returned:" << std::endl
			<< log << std::endl << std::endl;
		delete[] log;
	}

	return ok;
}
