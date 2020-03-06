#include <cstdlib>

#include <Ui/Window.h>

// Test hierarchcal Z buffer
bool testHzb();

int main(int argc, char** argv) {
	bool success = true;
	success = success && testHzb();
	return EXIT_SUCCESS;
}

bool testHzb()
{
	Window window(128, 128, "SandViewer Tests - HZB");
	/*
	// Loading
	loadGeometry();
	loadOcclusionProxy();

	// Render pipeline
	renderHzb();
	render();
	*/
	return true;
}
