#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

in VertexData {
    vec3 normal_ws;
    vec3 position_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
    vec3 baseColor;
} vert;

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

#include "include/standard-material.inc.glsl"
uniform StandardMaterial uMaterial[3];

uniform float uRoughness = 0.5;
uniform float uMetallic = 0.0;
uniform float uNormalMapping = 1.0;

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(172.0/255.0, 23.0/255.0, 1.0/255.0);

#include "include/random.inc.glsl"
#include "sand/procedural-color.inc.glsl"

uniform float normal_mapping = 1.0;

void main() {
    SurfacePoint surf = SurfacePoint(
        vert.position_ws,
        vert.normal_ws,
        vert.tangent_ws,
        vert.uv,
        normal_mapping
    );

    GFragment fragment = SampleStandardMaterial(uMaterial[vert.materialId], surf);

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    } else if (isUsingProceduralColor()) {
        fragment.baseColor = vert.baseColor.rgb;
    }

    autoPackGFragment(fragment);
}

