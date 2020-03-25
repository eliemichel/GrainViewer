#version 450 core
#include "sys:defines"

#pragma variant STAGE_EPSILON_ZBUFFER
#pragma variant NO_DISCARD_IN_EPSILON_ZBUFFER
#pragma variant NO_COLOR_OUTPUT
#pragma variant STAGE_EXTRA_INIT
#pragma variant USING_ShellCullingFragDepth // to use with FragDepth ShellCullingStrategy

#include "include/uniform/camera.inc.glsl"

in vec3 position_ws;
in vec3 baseColor;
in float radius;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;

#include "include/gbuffer.inc.glsl"
#include "include/zbuffer.inc.glsl"

const int cDebugShapeNone = -1;
const int cDebugShapeLitSphere = 0; // directly lit sphere, not using ad-hoc lighting
const int cDebugShapeDisc = 1;
const int cDebugShapeSquare = 2;
const int cDebugShapeNormalSphere = 3; // sphere colored with its normal
uniform int uDebugShape = cDebugShapeDisc;

const int cWeightNone = -1;
const int cWeightLinear = 0;
const int cWeightQuad = 1;
const int cWeightGaussian = 2;
uniform int uWeightMode = cWeightNone;

const int cShellCullingFragDepth = 0;
const int cShellCullingMoveAway = 1;
const int cShellCullingDepthRange = 2;
uniform int uShellCullingStrategy;

uniform float uEpsilon = 0.5;
uniform bool uShellDepthFalloff = false;

///////////////////////////////////////////////////////////////////////////////
#ifdef STAGE_EPSILON_ZBUFFER

// We need to use defines for toggling manipulation of gl_FragDepth, uniforms
// are not enough because the use of gl_FragDepth is statically detected and
// affects shader compilation.
#ifdef USING_ShellCullingFragDepth
layout (depth_greater) out float gl_FragDepth;
#endif // USING_ShellCullingFragDepth

void main() {
#ifndef NO_DISCARD_IN_EPSILON_ZBUFFER
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }
#endif // NO_DISCARD_IN_EPSILON_ZBUFFER

#ifdef USING_ShellCullingFragDepth
    // Readable version
    float linearDepth = linearizeDepth(gl_FragCoord.z);
    linearDepth += uEpsilon;
    float logDepth = unlinearizeDepth(linearDepth);
    gl_FragDepth = logDepth;
#endif // USING_ShellCullingFragDepth

#ifndef NO_COLOR_OUTPUT
    // Init for accumulation
    gbuffer_color1 = vec4(0.0);
#endif // NO_COLOR_OUTPUT
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(STAGE_EXTRA_INIT) // STAGE_EXTRA_INIT

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }

    // Init for accumulation
    gbuffer_color1 = vec4(0.0);
}

///////////////////////////////////////////////////////////////////////////////
#else // STAGE

uniform float height = 0.0;
uniform float metallic = 0.0;
uniform float roughness = 0.4;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;

uniform sampler2D uDepthTexture;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float sqDistToCenter = dot(uv, uv);
    if (uDebugShape != cDebugShapeSquare && sqDistToCenter > 1.0) {
        discard;
    }

    float weight;
    switch (uWeightMode) {
    case cWeightLinear:
        if (uDebugShape == cDebugShapeSquare) {
            weight = 1.0 - max(abs(uv.x), abs(uv.y));
        } else {
            weight = 1.0 - sqrt(sqDistToCenter);
        }
        break;
    case cWeightQuad:
        if (uDebugShape == cDebugShapeSquare) {
            float d = max(abs(uv.x), abs(uv.y));
            weight = 1.0 - d * d;
        } else {
            weight = 1.0 - sqDistToCenter;
        }
        break;
    case cWeightGaussian:
        weight = -1.0; // TODO
        break;
    case cWeightNone:
    default:
        weight = 1.0;
        break;
    }

    vec3 n;
    if (uDebugShape == cDebugShapeLitSphere || uDebugShape == cDebugShapeNormalSphere) {
        Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
        Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
        vec3 sphereHitPosition_ws;
        bool intersectSphere = intersectRaySphere(sphereHitPosition_ws, ray_ws, position_ws.xyz, radius);
        if (intersectSphere) {
            n = normalize(sphereHitPosition_ws - position_ws);
        } else {
            //discard;
            n = -ray_ws.direction;
        }
    } else {
        n = normalize(normal);
    }

    GFragment fragment;
    fragment.baseColor = baseColor.rgb;
    fragment.normal = n;
    fragment.ws_coord = position_ws;
    fragment.material_id = accumulatedPbrMaterial;
    fragment.roughness = roughness;
    fragment.metallic = metallic;
    fragment.emission = vec3(0.0);
    fragment.alpha = 1.0;
    fragment.count = 1;

    // direct shading
    if (uDebugShape != -1) {
        vec3 beauty = vec3(0.0, 0.0, 0.0);

        if (uDebugShape == cDebugShapeLitSphere) {
            // Ad-hoc lighting, just to test
            vec3 camPos_ws = vec3(inverseViewMatrix[3]);
            vec3 toCam = normalize(camPos_ws - position_ws);

            SurfaceAttributes surface;
            surface.baseColor = fragment.baseColor;
            surface.metallic = fragment.metallic;
            surface.roughness = fragment.roughness;
            surface.reflectance = 0.7;


            for (int k = 0 ; k < 1 ; ++k) {
                vec3 toLight = normalize(vec3(5.0, 5.0, 5.0) - position_ws);
                vec3 f = vec3(0.0);
                f = brdf(toCam, n, toLight, surface);
                beauty.rgb += f * vec3(3.0);
            }
        } else if (uDebugShape == cDebugShapeNormalSphere) {
            beauty.rgb = normalize(n) * 0.5 + 0.5;
        } else {
            beauty = baseColor;
        }

        if (uShellDepthFalloff) {
            float d = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
            float limitDepth = linearizeDepth(d);
            float depth = linearizeDepth(gl_FragCoord.z);
            float fac = (limitDepth - depth) / uEpsilon;
            //beauty = vec3(1.0, fac, 0.0);
            weight *= fac;
        }

        fragment.roughness = weight;
        fragment.ws_coord = beauty * weight;
    }

    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE_EPSILON_ZBUFFER
