#version 450 core
#include "sys:defines"

in GeometryData {
    vec3 position_ws;
    vec3 normal_ws;
    vec3 normal_cs;
    vec3 tangent_ws;
    vec2 uv;
    flat uint materialId;
} geo;

#include "include/standard-material.inc.glsl"
uniform StandardMaterial uMaterial[3];

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

uniform float normal_mapping = 1.0;

#define MAT uMaterial[geo.materialId]

void main() {
    GFragment fragment;

    // Normal
    vec3 normal = geo.normal_ws;
    if (normal_mapping > 0 && MAT.hasNormalMap) {
        vec3 normal_ts = vec3(texture(MAT.normalMap, geo.uv)) * 2.0 - 1.0;
        // Normal mapping
        mat3 TBN = mat3(
            normalize(cross(geo.normal_ws, geo.tangent_ws)),
            normalize(geo.tangent_ws),
            normalize(geo.normal_ws)
        );
        normal = normalize(geo.normal_ws * normal_mapping + TBN * normal_ts);
    }
    fragment.normal = normalize(normal);

    // Base Color
    if (MAT.hasBaseColorMap) {
        fragment.baseColor = texture(MAT.baseColorMap, geo.uv).rgb;
    } else {
        fragment.baseColor = MAT.baseColor;
    }

    // Metallic/Roughness
    if (MAT.hasMetallicRoughnessMap) {
        vec4 t = texture(MAT.metallicRoughnessMap, geo.uv);
        fragment.metallic = t.x;
        fragment.roughness = t.y;
    } else {
        if (MAT.hasMetallicMap) {
            fragment.metallic = texture(MAT.metallicMap, geo.uv).r;
        } else {
            fragment.metallic = MAT.metallic;
        }
        if (MAT.hasRoughnessMap) {
            fragment.roughness = texture(MAT.roughnessMap, geo.uv).r;
        } else {
            fragment.roughness = MAT.roughness;
        }
    }
    
    // Other attributes
    fragment.ws_coord = geo.position_ws;
    fragment.material_id = pbrMaterial;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;

    autoPackGFragment(fragment);
}
