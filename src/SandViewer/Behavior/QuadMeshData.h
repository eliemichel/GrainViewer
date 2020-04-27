
#pragma once

#include <OpenGL>
#include "Behavior.h"
#include "GlBuffer.h"
#include <refl.hpp>
#include <memory>


class QuadMeshData : public Behavior {
public:
	// Behavior implementation
	void start() override;
	void onDestroy() override;

	void draw() const;

private:
	std::unique_ptr<GlBuffer> m_vbo;
	GLuint m_vao;
};

registerBehaviorType(QuadMeshData)
