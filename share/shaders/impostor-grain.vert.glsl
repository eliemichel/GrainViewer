#version 430 core
#include "sys:defines"

#pragma varopt PASS_BLIT_TO_MAIN_FBO

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

#include "include/standard-posteffect.vert.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

out VertexData {
	uint id;
} vert;

// All the vertex shader is in the geometry shader to allow for whole poitn discard
void main() {
	vert.id = gl_VertexID;
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
