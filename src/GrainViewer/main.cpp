/**
 * This file is part of GrainViewer
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

#include <OpenGL>

#include "Ui/Window.h"
#include "Ui/Gui.h"
#include "Scene.h"
#include "GlobalTimer.h"

#include <cstdlib> // for EXIT_FAILURE and EXIT_SUCCESS
#include <memory>

// Force running on nvidia chip if available
#ifdef _WIN32
extern "C" {
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif // _WIN32

int main(int argc, char *argv[]) {
	const char *title = "Grain Viewer -- Copyright (c) 2017 - 2020 -- Telecom Paris (Elie Michel, CG Group)";

	std::string filename = "scene.json";
	if (argc >= 2) {
		filename = std::string(argv[1]);
	}

	auto window = std::make_shared<Window>(1280, 720, title);
	if (!window->isValid()) {
		return EXIT_FAILURE;
	}

	auto gui = std::make_unique<Gui>(window);
	auto scene = std::make_shared<Scene>();

	gui->setScene(scene);

	gui->beforeLoading();
	if (!scene->load(filename)) {
		return EXIT_FAILURE;
	}
	gui->afterLoading();

	while (!window->shouldClose()) {
		GlobalTimer::StartFrame();
		window->pollEvents();

		gui->update();
		gui->render();

		window->swapBuffers();
		GlobalTimer::StopFrame();
	}

	return EXIT_SUCCESS;
}
