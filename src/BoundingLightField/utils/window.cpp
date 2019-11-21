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

#include <glad/glad.h> // must be the first include

#include "window.h"
#include "debug.h"

#include <iostream>

GLFWwindow * startup() {
	GLFWwindow *window = nullptr;
	int width = 800, height = 600;
	const char *title = "AugenLight v1.0 / Level 0 -- Copyright (c) 2017 - 2018 -- Élie Michel";

	// Initialize GLFW, the library responsible for window management
	if (!glfwInit()) {
		std::cerr << "ERROR: Failed to init GLFW" << std::endl;
		return nullptr;
	}

	// Before creating the window, set some option flags
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#if _DEBUG
	// Enable opengl debug output when building in debug mode
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	// Create the window
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		std::cerr << "ERROR: Failed to open window" << std::endl;
		glfwTerminate();
		return nullptr;
	}

	// Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERROR: Failed to initialize OpenGL context" << std::endl;
		glfwTerminate();
		return nullptr;
	}

#ifndef NDEBUG
	enableGlDebug();
#endif // !NDEBUG

	std::cout << "INFO: Running over OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;

	return window;
}


void shutdown(GLFWwindow *window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}
