#version 450 core
#include "sys:defines"

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

#include "include/standard-posteffect.geo.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

layout (points) in;
layout (points, max_vertices = 1) out;

in VertexData {
	flat uint id;
} vert[];

out GeometryData {
	flat uint id;
	float radius;
	vec3 position_ws;
	mat4 gs_from_ws;
} geo;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform float uGrainRadius = 0.005;

#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"
#include "include/sprite.inc.glsl"
#include "sand/random-grains.inc.glsl"

void main() {
	geo.id = vert[0].id;
	geo.radius = uGrainRadius;

    geo.position_ws = (modelMatrix * gl_in[0].gl_Position).xyz;
    vec4 position_cs = viewMatrix * vec4(geo.position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(geo.radius, gl_Position);

    geo.gs_from_ws = randomGrainMatrix(int(geo.id), geo.position_ws);

    EmitVertex();
    EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS_BLIT_TO_MAIN_FBO
