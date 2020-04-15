#version 450 core
#include "sys:defines"

in VertexData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
    flat int layer;
} vert[];

out GeometryData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
} geo;

uniform uint uReducedViewCount = 1;

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/gbuffer2.inc.glsl"
#include "include/impostor.inc.glsl"
uniform mat4 uProjectionMatrix;

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

// this is just a passthrough geo shader
void main() {
	int layer = vert[0].layer;
	gl_Layer = layer;
	mat4 bakingViewMatrix = (InverseBakingViewMatrix(uint(layer), uReducedViewCount));

	for (int i = 0; i < gl_in.length(); i++) {
		geo.position_ws = vert[i].position_ws;
		geo.normal_ws = vert[i].normal_ws;
		geo.tangent_ws = vert[i].tangent_ws;
		geo.uv = vert[i].uv;
		geo.materialId = vert[i].materialId;
		gl_Position = uProjectionMatrix * bakingViewMatrix * vec4(geo.position_ws, 1.0);
		gl_Position.y *= -1;
		EmitVertex();
	}
	EndPrimitive();
}
