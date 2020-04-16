#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>
#include <memory>

/**
 * A large quad covering the whole screen
 */
class PostEffect
{
public:
	// Use static instance
	static void Draw(bool disableDepthTest = true);
	static void DrawInstanced(int n);
	static void DrawWithDepthTest() { Draw(false); }

public:
	PostEffect();
	~PostEffect();
	void draw(bool disableDepthTest = true, int instances = 1);

private:
	GLuint m_vao;
	GLuint m_vbo;

private:
	static std::unique_ptr<PostEffect> s_instance;
};
