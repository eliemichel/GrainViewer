#version 450 core
#include "sys:defines"

#pragma variant PROCEDURAL_BASECOLOR

in VertexData {
    vec3 normal_ws;
    vec3 position_ws;
    vec3 tangent_ws;
    vec2 uv_ts;
    flat uint matId;
    vec3 baseColor;
} vert;

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

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(172.0/255.0, 23.0/255.0, 1.0/255.0);

#include "include/random.inc.glsl"
#include "sand/procedural-color.inc.glsl"

void main() {
    vec3 normal = vert.normal_ws;
    if (uNormalMapping > 0 && uMaterial[vert.matId].hasNormalMap) {
        vec3 normal_ts = vec3(texture(uMaterial[vert.matId].normalMap, vert.uv_ts)) * 2.0 - 1.0;
        // Normal mapping
        mat3 TBN = mat3(
            normalize(cross(vert.normal_ws, vert.tangent_ws)),
            normalize(vert.tangent_ws),
            normalize(vert.normal_ws)
        );
        normal = normalize(vert.normal_ws * uNormalMapping + TBN * normal_ts);
    }

    GFragment fragment;
    fragment.baseColor = vert.baseColor.rgb;
    if (!isUsingProceduralColor()) {
        if (uMaterial[vert.matId].hasBaseColorMap) {
            fragment.baseColor = texture(uMaterial[vert.matId].baseColorMap, vert.uv_ts).rgb;
        } else {
            fragment.baseColor = uMaterial[vert.matId].baseColor;
        }
    }

    fragment.roughness = uRoughness;
    fragment.metallic = uMetallic;
    if (uMaterial[vert.matId].hasMetallicRoughnessMap) {
        vec4 t = texture(uMaterial[vert.matId].metallicRoughnessMap, vert.uv_ts);
        fragment.metallic = t.x;
        fragment.roughness = t.y;
    } else {
        fragment.roughness = uMaterial[vert.matId].roughness;
        fragment.metallic = uMaterial[vert.matId].metallic;
    }
    fragment.normal = normalize(normal);
    fragment.ws_coord = vert.position_ws;
    fragment.material_id = pbrMaterial;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    }

    autoPackGFragment(fragment);
}

