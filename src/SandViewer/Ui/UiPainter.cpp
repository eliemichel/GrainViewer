#include "UiPainter.h"

std::unique_ptr<QuadMeshData> UiPainter::s_quad = nullptr;

QuadMeshData& UiPainter::Quad()
{
	if (!s_quad) {
		s_quad = std::make_unique<QuadMeshData>();
		s_quad->start();
	}
	return *s_quad;
}

void UiPainter::DrawQuad()
{
	Quad().draw();
}
