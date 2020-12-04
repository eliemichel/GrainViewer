#version 450 core
#include "sys:defines"

in vec3 position_ws;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;

uniform vec4 baseColor = vec4(0.0, 0.0, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 0.0;
uniform float roughness = 0.0;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.1, 0.1, 0.1);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;

#include "include/gbuffer.inc.glsl"

void main() {
    GFragment fragment;
    fragment.baseColor = baseColor.rgb;
    fragment.normal = normalize(normal * 2. - 1.);
    fragment.ws_coord = position_ws;
    fragment.material_id = pbrMaterial;
    fragment.roughness = roughness;
    fragment.metallic = metallic;
    fragment.emission = emission;
    fragment.alpha = 1.0;
    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}
