#version 450 core
#include "sys:defines"

in vec3 position_ws;
in vec3 normal_ws;
in vec3 normal_cs;
in vec3 tangent_ws;
in vec2 uv0_ts;
in vec2 uv1_ts;
in vec3 color0_srgb;

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

uniform vec4 baseColor = vec4(1.0, 0.5, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 1.0;
uniform float roughness = 0.7;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 1.0;

#include "include/gbuffer.inc.glsl"

void main() {
    int matId = 0;
    vec3 normal = normal_ws;
    if (normal_mapping > 0 && material[matId].hasNormalMap) {
        vec3 normal_ts = vec3(texture(material[matId].normalMap, uv0_ts)) * 2.0 - 1.0;
        // Normal mapping
        mat3 TBN = mat3(
            normalize(cross(normal_ws, tangent_ws)),
            normalize(tangent_ws),
            normalize(normal_ws)
        );
        normal = normalize(normal_ws * normal_mapping + TBN * normal_ts);
    }

    GFragment fragment;
    fragment.baseColor = baseColor.rgb;
    if (material[matId].hasBaseColorMap) {
        fragment.baseColor = texture(material[matId].baseColorMap, uv0_ts).rgb;
    } else {
        fragment.baseColor = material[matId].baseColor;
    }
    fragment.roughness = roughness;
    fragment.metallic = metallic;
    if (material[matId].hasMetallicRoughnessMap) {
        vec4 t = texture(material[matId].metallicRoughnessMap, uv0_ts);
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
