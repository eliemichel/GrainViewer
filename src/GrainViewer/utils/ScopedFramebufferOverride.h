#pragma once

#ifdef _WIN32
#include <windows.h> // Avoid issue with APIENTRY redefinition in Glad
#endif // _WIN32

#include <glad/modernglad.h>

/**
 * Restore framebuffer bindings upon destruction
 */
class ScopedFramebufferOverride {
public:
	ScopedFramebufferOverride() {
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_drawFboId);
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_readFboId);
	}
	~ScopedFramebufferOverride() {
		restore();
	}
	void restore() {
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_drawFboId);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFboId);
	}

private:
	GLint m_drawFboId = 0;
	GLint m_readFboId = 0;
};
