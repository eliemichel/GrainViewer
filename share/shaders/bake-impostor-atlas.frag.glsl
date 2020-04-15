#version 450 core
#include "sys:defines"

in GeometryData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
} geo;

layout (location = 0) out vec4 out_normalAlpha;
layout (location = 1) out vec4 out_baseColor;
layout (location = 2) out vec4 out_metallicRoughness;

#include "include/gbuffer2.inc.glsl"
#include "include/standard-material.inc.glsl"
uniform StandardMaterial uMaterial[3];

uniform float uNormalMapping = 10.0;

void main() {
    SurfacePoint surf = SurfacePoint(
        geo.position_ws,
        geo.normal_ws,
        geo.tangent_ws,
        geo.uv,
        uNormalMapping
    );

    GFragment fragment = SampleStandardMaterial(uMaterial[geo.materialId], surf);

    out_normalAlpha.rgb = normalize(fragment.normal.xyz) * 0.5 + 0.5;
    out_normalAlpha.a = 1.0;
    out_baseColor.rgb = fragment.baseColor.rgb;
    out_metallicRoughness.x = fragment.metallic;
    out_metallicRoughness.y = fragment.roughness;
}
