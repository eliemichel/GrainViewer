#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

in vec3 normal_ws;
in vec3 position_ws;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;
in vec3 baseColor;

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

uniform struct Material {
    vec3 baseColor;
    float metallic;
    float roughness;
    sampler2D baseColorMap;
    sampler2D normalMap;
    sampler2D metallicRoughnessMap;
    bool hasBaseColorMap;
    bool hasNormalMap;
    bool hasMetallicRoughnessMap;
} uMaterial[3];

uniform float uRoughness = 0.5;
uniform float uMetallic = 0.0;
uniform float uNormalMapping = 1.0;

void main() {
    vec3 normal = normal_ws;
    if (uNormalMapping > 0 && uMaterial[matId].hasNormalMap) {
        vec3 normal_ts = vec3(texture(uMaterial[matId].normalMap, uv_ts)) * 2.0 - 1.0;
        // Normal mapping
        mat3 TBN = mat3(
            normalize(cross(normal_ws, tangent_ws)),
            normalize(tangent_ws),
            normalize(normal_ws)
        );
        normal = normalize(normal_ws * uNormalMapping + TBN * normal_ts);
    }

    GFragment fragment;
#ifdef PROCEDURAL_BASECOLOR
    fragment.baseColor = baseColor.rgb;
#else // PROCEDURAL_BASECOLOR
    fragment.baseColor = baseColor.rgb;
    if (uMaterial[matId].hasBaseColorMap) {
        fragment.baseColor = texture(uMaterial[matId].baseColorMap, uv_ts).rgb;
    } else {
        fragment.baseColor = uMaterial[matId].baseColor;
    }
#endif // PROCEDURAL_BASECOLOR

    fragment.roughness = uRoughness;
    fragment.metallic = uMetallic;
    if (uMaterial[matId].hasMetallicRoughnessMap) {
        vec4 t = texture(uMaterial[matId].metallicRoughnessMap, uv_ts);
        fragment.metallic = t.x;
        fragment.roughness = t.y;
    } else {
        fragment.roughness = uMaterial[matId].roughness;
        fragment.metallic = uMaterial[matId].metallic;
    }
    fragment.normal = normalize(normal);
    fragment.ws_coord = position_ws;
    fragment.material_id = pbrMaterial;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;

    autoPackGFragment(fragment);
}

