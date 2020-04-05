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

#ifdef PRECOMPUTE_IN_VERTEX
#include "include/raytracing.inc.glsl"
#include "include/impostor.inc.glsl"
#endif // PRECOMPUTE_IN_VERTEX

void main() {
	geo.id = vert[0].id;
	geo.radius = uGrainRadius;

    geo.position_ws = (modelMatrix * gl_in[0].gl_Position).xyz;
    vec4 position_cs = viewMatrix * vec4(geo.position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(geo.radius, gl_Position);

    geo.gs_from_ws = randomGrainMatrix(int(geo.id), geo.position_ws);

#ifdef PRECOMPUTE_IN_VERTEX
	Ray ray_ws;
	ray_ws.origin = (inverseViewMatrix * vec4(0, 0, 0, 1)).xyz;
	ray_ws.direction = normalize(geo.position_ws - ray_ws.origin);
    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

    fragment = SampleImpostor(impostor[0], ray_gs, geo.radius);
	mat3 ws_from_gs_rot = transpose(mat3(geo.gs_from_ws));
    fragment.normal = ws_from_gs_rot * fragment.normal;

#endif // PRECOMPUTE_IN_VERTEX

    EmitVertex();
    EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS_BLIT_TO_MAIN_FBO
