#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

in vec3 normal_ws;
in vec3 position_ws;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;
in vec3 baseColor;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;

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
} material[3];

uniform float roughness = 0.5;
uniform float metallic = 0.0;
uniform float uNormalMapping = 1.0;

#include "include/gbuffer.inc.glsl"

void main() {
    vec3 normal = normal_ws;
    if (uNormalMapping > 0 && material[matId].hasNormalMap) {
        vec3 normal_ts = vec3(texture(material[matId].normalMap, uv_ts)) * 2.0 - 1.0;
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
    if (material[matId].hasBaseColorMap) {
        fragment.baseColor = texture(material[matId].baseColorMap, uv_ts).rgb;
    } else {
        fragment.baseColor = material[matId].baseColor;
    }
#endif // PROCEDURAL_BASECOLOR

    fragment.roughness = roughness;
    fragment.metallic = metallic;
    if (material[matId].hasMetallicRoughnessMap) {
        vec4 t = texture(material[matId].metallicRoughnessMap, uv_ts);
        fragment.metallic = t.x;
        fragment.roughness = t.y;
    } else {
        fragment.roughness = material[matId].roughness;
        fragment.metallic = material[matId].metallic;
    }
    fragment.normal = normalize(normal);
    fragment.ws_coord = position_ws;
    fragment.material_id = pbrMaterial;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;

    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}

