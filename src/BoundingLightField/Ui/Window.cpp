#include <glad/glad.h>
#include <iostream>
#include <GLFW/glfw3.h>

#include "utils/debug.h"
#include "Window.h"

Window::Window(int width, int height, const char *title)
	: m_window(nullptr)
{
	// Initialize GLFW, the library responsible for window management
	if (!glfwInit()) {
		std::cerr << "ERROR: Failed to init GLFW" << std::endl;
		return;
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
	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!m_window) {
		std::cerr << "ERROR: Failed to open window" << std::endl;
		glfwTerminate();
		return;
	}

	// Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
	glfwMakeContextCurrent(m_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "ERROR: Failed to initialize OpenGL context" << std::endl;
		glfwTerminate();
		return;
	}

#ifndef NDEBUG
	enableGlDebug();
#endif // !NDEBUG

	std::cout << "INFO: Running over OpenGL " << GLVersion.major << "." << GLVersion.minor << std::endl;
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

GLFWwindow * Window::glfw() const
{
	return m_window;
}

bool Window::isValid() const
{
	return m_window != nullptr;
}
