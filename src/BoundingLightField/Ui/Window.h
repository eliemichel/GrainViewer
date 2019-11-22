#pragma once

struct GLFWwindow;

/**
 * Simple wrapper around GLFWwindow
 * also creating an OpenGL 4.5 context using glad
 */
class Window {
public:
	Window(int width, int height, const char *title);
	~Window();

	/**
	 * @return the raw GLFWwindow object, or nullptr if initialization failed.
	 */
	GLFWwindow *glfw() const;

	/**
	 * @return true iff window has correctly been initialized
	 */
	bool isValid() const;

private:
	GLFWwindow *m_window = nullptr;
};
