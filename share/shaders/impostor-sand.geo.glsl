#version 450 core
#include "sys:defines"

#pragma varopt PASS_BLIT_TO_MAIN_FBO
#pragma opt PRECOMPUTE_IN_VERTEX

///////////////////////////////////////////////////////////////////////////////
#ifdef PASS_BLIT_TO_MAIN_FBO

#include "include/standard-posteffect.geo.inc.glsl"

///////////////////////////////////////////////////////////////////////////////
#else // PASS_BLIT_TO_MAIN_FBO

layout(points) in;
layout(points, max_vertices = 1) out;

struct PointCloundVboEntry {
    vec4 position;
};
layout(std430, binding = 0) restrict readonly buffer pointsSsbo {
    PointCloundVboEntry pointVertexAttributes[];
};
layout (std430, binding = 1) restrict readonly buffer pointElementsSsbo {
    uint pointElements[];
};

in VertexData {
	uint id;
} vert[];

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

uniform sampler2D uOcclusionMap;
uniform bool uUseOcclusionMap = false;

uniform int uPrerenderSurfaceStep = 0; // 0: prerender, 1: render remaining
uniform bool uPrerenderSurface = false;

#include "include/anim.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"
#include "include/sprite.inc.glsl"
#include "sand/random-grains.inc.glsl"

#ifdef PRECOMPUTE_IN_VERTEX
#include "include/raytracing.inc.glsl"
#include "include/gbuffer2.inc.glsl"
#include "include/impostor.inc.glsl"
#include "include/zbuffer.inc.glsl"
uniform SphericalImpostor uImpostor[3];
#endif // PRECOMPUTE_IN_VERTEX

void main() {
	geo.id =
        uUsePointElements
        ? pointElements[vert[0].id]
        : vert[0].id;

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

    geo.id = animPointId%20; // WTF?
    geo.gs_from_ws = randomGrainMatrix(int(geo.id), geo.position_ws);

	if (uPrerenderSurface && uUseOcclusionMap) {
		vec2 fragCoord = resolution.xy * (gl_Position.xy / gl_Position.w * 0.5 + 0.5);
		fragCoord = clamp(fragCoord, vec2(0.5), resolution.xy - vec2(0.5));
		vec3 closerGrain_cs = texelFetch(uOcclusionMap, ivec2(fragCoord.xy), 0).xyz;
		//bool isSurface = linearizeDepth(position_cs.z) < linearizeDepth(closerGrain_cs.z) - geo.radius * 0.01;
		bool isSurface = position_cs.z < closerGrain_cs.z;
		if (uPrerenderSurfaceStep == 0 && isSurface) return;
		if (uPrerenderSurfaceStep == 1 && !isSurface) return;
	}

#ifdef PRECOMPUTE_IN_VERTEX
	Ray ray_ws;
	ray_ws.origin = (inverseViewMatrix * vec4(0, 0, 0, 1)).xyz;
	ray_ws.direction = normalize(geo.position_ws - ray_ws.origin);
    Ray ray_gs = TransformRay(ray_ws, geo.gs_from_ws);

    uint n = uImpostor[0].viewCount;
#  ifdef NO_INTERPOLATION
	geo.i.x = DirectionToViewIndex(-ray_gs.direction, n);
#  else // NO_INTERPOLATION
	DirectionToViewIndices(-ray_gs.direction, n, geo.i, geo.alpha);
#  endif // NO_INTERPOLATION
#endif // PRECOMPUTE_IN_VERTEX

	EmitVertex();
	EndPrimitive();
}

///////////////////////////////////////////////////////////////////////////////
#endif // PASS
