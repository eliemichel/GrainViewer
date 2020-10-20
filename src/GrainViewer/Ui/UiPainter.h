#pragma once

#include "Behavior/QuadMeshData.h"
#include <memory>

/**
 * Utils for drawing UI without using ImGui
 */
class UiPainter
{
private:
	static std::unique_ptr<QuadMeshData> s_quad;
	static QuadMeshData& Quad();

public:
	static void DrawQuad();
};

