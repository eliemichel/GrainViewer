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
#include <cstdlib> // for EXIT_FAILURE and EXIT_SUCCESS
#include <memory>
#include <GLFW/glfw3.h>

#include "Ui/Window.h"
#include "Ui/Gui.h"
#include "Scene.h"

int main(int argc, char *argv[]) {
	const char *title = "Bounding Light Field -- Copyright (c) 2019 -- CG Group @ Telecom Paris";

	std::string filename = "scene.json";
	if (argc >= 2) {
		filename = std::string(argv[1]);
	}

	auto window = std::make_shared<Window>(1000, 600, title);
	if (!window->isValid()) {
		return EXIT_FAILURE;
	}

	auto scene = std::make_shared<Scene>();
	if (!scene->load(filename)) {
		return EXIT_FAILURE;
	}

	auto gui = std::make_unique<Gui>(window, scene);

	while (!glfwWindowShouldClose(window->glfw())) {
		glfwPollEvents();

		gui->update();
		gui->render();

		glfwSwapBuffers(window->glfw());
	}

	return EXIT_SUCCESS;
}
