#version 450 core
#include "sys:defines"

struct PointCloundVboEntry {
	vec4 position;
};
layout(std430, binding = 1) restrict readonly buffer points {
	PointCloundVboEntry vbo[];
};

out Geometry {
	vec4 position_cs;
	float radius;
} geo;

uniform float uOuterOverInnerRadius = 1.0 / 0.5;
uniform float uGrainRadius;
uniform float uGrainInnerRadiusRatio;
uniform float uOccluderMapSpriteScale = 0.2;

uniform uint uFrameCount;
uniform uint uPointCount;
uniform float uTime;
uniform float uFps = 25.0;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "../include/uniform/camera.inc.glsl"

#include "../include/utils.inc.glsl"
#include "../include/sprite.inc.glsl"
#include "../include/anim.inc.glsl"

void main() {
	uint pointId = AnimatedPointId2(gl_VertexID, uFrameCount, uPointCount, uTime, uFps);

	vec4 position_ms = vec4(vbo[pointId].position.xyz, 1.0);
	geo.position_cs = viewModelMatrix * position_ms;
	geo.radius = uGrainRadius * uGrainInnerRadiusRatio; // inner radius
	gl_Position = projectionMatrix * geo.position_cs;
	// The *.15 has no explaination, but it empirically increases occlusion culling
	gl_PointSize = SpriteSize(geo.radius, gl_Position) * uOccluderMapSpriteScale;
}

