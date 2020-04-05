#version 430 core
#include "sys:defines"

#pragma varopt PASS_BLIT_TO_MAIN_FBO

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

#include "include/standard-posteffect.vert.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) restrict readonly buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) restrict readonly buffer pointElementsSsbo {
    uint pointElements[];
};

out GeometryData {
	flat uint id;
	float radius;
	vec3 position_ws;
	mat4 gs_from_ws;
#ifdef PRECOMPUTE_IN_VERTEX
	flat uvec4 i;
	vec2 alpha;
#endif // PRECOMPUTE_IN_VERTEX
} geo;

uniform uint uFrameCount = 1;
uniform uint uPointCount;
uniform float uFps = 25.0;
uniform float uTime;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

uniform bool uUseAnimation = true;
uniform bool uUsePointElements = true;

uniform float uGrainRadius;

#include "include/anim.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"
#include "include/sprite.inc.glsl"
#include "sand/random-grains.inc.glsl"

#ifdef PRECOMPUTE_IN_VERTEX
#include "include/raytracing.inc.glsl"
#include "include/gbuffer2.inc.glsl"
#include "include/impostor.inc.glsl"
uniform SphericalImpostor impostor[3];
#endif // PRECOMPUTE_IN_VERTEX

void main() {
	geo.id =
        uUsePointElements
        ? pointElements[gl_VertexID]
        : gl_VertexID;

    uint animPointId =
        uUseAnimation
        ? AnimatedPointId2(geo.id, uFrameCount, uPointCount, uTime, uFps)
        : geo.id;

	vec3 p = pointVertexAttributes[animPointId].position.xyz;

    geo.radius = uGrainRadius;

    geo.position_ws = (modelMatrix * vec4(p, 1.0)).xyz;
    vec4 position_cs = viewMatrix * vec4(geo.position_ws, 1.0);

    gl_Position = projectionMatrix * position_cs;
    
    gl_PointSize = SpriteSize(geo.radius, gl_Position);

    geo.gs_from_ws = randomGrainMatrix(int(geo.id), geo.position_ws);

#ifdef PRECOMPUTE_IN_VERTEX
	Ray ray_ws;
	ray_ws.origin = (inverseViewMatrix * vec4(0, 0, 0, 1)).xyz;
	ray_ws.direction = normalize(geo.position_ws - ray_ws.origin);
    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

    uint n = impostor[0].viewCount;
	DirectionToViewIndices(-ray_gs.direction, n, geo.i, geo.alpha);
#endif // PRECOMPUTE_IN_VERTEX
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
