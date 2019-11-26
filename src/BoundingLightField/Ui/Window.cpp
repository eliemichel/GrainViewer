#ifdef _WIN32
#include <windows.h>
#endif

#include <glad/glad.h>
#include <iostream>
#include <GLFW/glfw3.h>

#include "utils/debug.h"
#include "Window.h"
#include "Logger.h"

Window::Window(int width, int height, const char *title)
	: m_window(nullptr)
{
	// Initialize GLFW, the library responsible for window management
	if (!glfwInit()) {
		ERR_LOG << "Failed to init GLFW";
		return;
	}

	// Before creating the window, set some option flags
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
#ifndef NDEBUG
	// Enable opengl debug output when building in debug mode
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif // !NDEBUG

	// Create the window
	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!m_window) {
		ERR_LOG << "Failed to open window";
		glfwTerminate();
		return;
	}

	// Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
	glfwMakeContextCurrent(m_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		ERR_LOG << "Failed to initialize OpenGL context";
		glfwTerminate();
		return;
	}

#ifndef NDEBUG
	enableGlDebug();
#endif // !NDEBUG

	LOG << "Running over OpenGL " << GLVersion.major << "." << GLVersion.minor;
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
