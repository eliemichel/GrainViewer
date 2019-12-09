#version 450 core
#include "sys:defines"

#pragma variant SHAPE_SPHERE

in vec3 position_ws;
in vec3 baseColor;
in float radius;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;

uniform float height = 0.0;
uniform float metallic = 0.0;
uniform float roughness = 0.4;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;
#include "include/uniform/camera.inc.glsl"

#include "include/gbuffer.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"

void main() {
#ifdef SHAPE_SPHERE
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
    vec3 sphereHitPosition_ws;
    bool intersectSphere = intersectRaySphere(sphereHitPosition_ws, ray_ws, position_ws.xyz, radius);
    if (!intersectSphere) {
        discard;
    }

    vec3 n = normalize(sphereHitPosition_ws - position_ws);
#else // SHAPE_SPHERE
    vec3 n = normalize(normal);
#endif // SHAPE_SPHERE

    GFragment fragment;
    fragment.baseColor = baseColor.rgb;
    fragment.normal = n;
    fragment.ws_coord = position_ws;
    fragment.material_id = pbrMaterial;
    fragment.roughness = roughness;
    fragment.metallic = metallic;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;
    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}
