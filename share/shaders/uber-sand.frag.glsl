#version 450 core
#include "sys:defines"

#pragma varopt PASS_DEPTH PASS_EPSILON_DEPTH PASS_BLIT_TO_MAIN_FBO
#pragma opt SHELL_CULLING

const int cDebugShapeNone = -1;
const int cDebugShapeRaytracedSphere = 0; // directly lit sphere, not using ad-hoc lighting
const int cDebugShapeDisc = 1;
const int cDebugShapeSquare = 2;
uniform int uDebugShape = cDebugShapeDisc;

const int cWeightNone = -1;
const int cWeightLinear = 0;
uniform int uWeightMode = cWeightNone;

uniform float uEpsilon = 0.5;
uniform bool uShellDepthFalloff = false;

uniform bool uCheckboardSprites;
uniform bool uShowSampleCount;

uniform sampler2D uDepthTexture;

uniform float uRadius;

#include "include/uniform/camera.inc.glsl"

#include "include/zbuffer.inc.glsl"

// Compute influence weight of a grain fragment
// sqDistToCenter = dot(uv, uv);
float computeWeight(vec2 uv, float sqDistToCenter) {
    float depthWeight = 1.0;
    if (uShellDepthFalloff) {
        float d = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
        float limitDepth = linearizeDepth(d);
        float depth = linearizeDepth(gl_FragCoord.z);
        float fac = (limitDepth - depth) / uEpsilon;
        depthWeight = fac;
    }

    float blendWeight = 1.0;
    switch (uWeightMode) {
    case cWeightLinear:
        if (uDebugShape == cDebugShapeSquare) {
            blendWeight = 1.0 - max(abs(uv.x), abs(uv.y));
            break;
        } else {
            blendWeight = 1.0 - sqrt(sqDistToCenter);
            break;
        }
    case cWeightNone:
    default:
        blendWeight = 1.0;
        break;
    }

    return depthWeight * blendWeight;
}

///////////////////////////////////////////////////////////////////////////////
#if defined(PASS_BLIT_TO_MAIN_FBO)

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

// depth attachement of the target fbo
// (roughly safe to use despite feedback loops because we know that there is
// exactly one fragment per pixel)
//uniform sampler2D uDepthTexture; // nope, depth test used as is, just write to gl_FragDepth

// attachments of the secondary fbo
uniform sampler2D uFboColor0Texture;
uniform sampler2D uFboColor1Texture;
uniform sampler2D uFboDepthTexture;

// Problem: semi transparency requires to read from current color attachment
// but this creates dangerous feedback loops
// (glBlend cannot be used because of packing)

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"

// This stage gathers additive measures and turn them into regular g-buffer fragments
void main() {
    float d = texelFetch(uFboDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
    gl_FragDepth = d;
    // add back uEpsilon here -- benchmark-me
    //gl_FragDepth = unlinearizeDepth(linearizeDepth(d) - uEpsilon);

    vec4 in_color = texelFetch(uFboColor0Texture, ivec2(gl_FragCoord.xy), 0);
    vec4 in_normal = texelFetch(uFboColor1Texture, ivec2(gl_FragCoord.xy), 0);
    float weightNormalization = 1.0 / in_color.a; // inverse sum of integration weights

    GFragment fragment;
    initGFragment(fragment);

    fragment.baseColor = in_color.rgb * weightNormalization;
    fragment.material_id = pbrMaterial;
    fragment.normal = in_normal.xyz * weightNormalization;

    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    vec3 cs_coord = (linearizeDepth(d) - uEpsilon - 0.*uRadius) * ray_cs.direction;
    fragment.ws_coord = (inverseViewMatrix * vec4(cs_coord, 1.0)).xyz;

    if (uShowSampleCount) {
        fragment.material_id = colormapDebugMaterial;
        fragment.baseColor = vec3(in_color.a);
    }

    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(PASS_EPSILON_DEPTH)

layout (location = 0) out vec4 color0;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }
}

///////////////////////////////////////////////////////////////////////////////
#else // DEFAULT PASS

#ifdef SHELL_CULLING
layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
#else // SHELL_CULLING
#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"
#endif // SHELL_CULLING

in FragmentData {
    vec3 position_ws;
    vec3 baseColor;
    float radius;
    float screenSpaceDiameter;
} inData;

uniform float height = 0.0;
uniform float metallic = 0.0;
uniform float roughness = 1.4;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;

uniform float uTime;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float sqDistToCenter = dot(uv, uv);

    float antialiasing = 1.0;
    float weight = 1.0;
#ifdef SHELL_CULLING
    weight = computeWeight(uv, sqDistToCenter);
    if (uDebugShape != cDebugShapeSquare) {
        antialiasing = smoothstep(1.0, 1.0 - 5.0 / inData.screenSpaceDiameter, sqDistToCenter);
    }
    weight *= antialiasing;
#else // SHELL_CULLING
    vec4 out_color = vec4(0.0); // mock outputs
    vec4 out_normal = vec4(0.0);
    if (uDebugShape != cDebugShapeSquare && sqDistToCenter > 1.0) {
        discard;
    }
#endif // SHELL_CULLING

    vec3 n;
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
    if (uDebugShape == cDebugShapeRaytracedSphere) {
        vec3 sphereHitPosition_ws;
        bool intersectSphere = intersectRaySphere(sphereHitPosition_ws, ray_ws, inData.position_ws.xyz, inData.radius);
        if (intersectSphere) {
            n = normalize(sphereHitPosition_ws - inData.position_ws);
        } else {
            //discard;
            n = -ray_ws.direction;
        }
        //n = mix(-ray_ws.direction, n, antialiasing);
    } else if (uDebugShape == cDebugShapeDisc) {
        if (antialiasing > 0.0) {
            n = mat3(inverseViewMatrix) * vec3(uv, sqrt(1.0 - sqDistToCenter));
            n = mix(-ray_ws.direction, n, antialiasing);
        } else {
            n = -ray_ws.direction;
        }
        //n = mat3(inverseViewMatrix) * vec3(uv, sqrt(max(0.0, 1.0 - sqDistToCenter)));
    } else {
        n = mat3(inverseViewMatrix) * normalize(-ray_cs.direction);
    }

    vec3 baseColor = inData.baseColor;

    // DEBUG checkerboard for mipmap test
    if (uCheckboardSprites) {
        baseColor = vec3(abs(step(uv.x, 0.0) - step(uv.y, 0.0)));
    }

    out_color.rgb = baseColor * weight;
    out_color.a = weight;
    out_normal.xyz = n * weight;

    if (uShowSampleCount) {
        out_color.a = clamp(ceil(antialiasing), 0, 1);
    }

#ifndef SHELL_CULLING
    // If not using shell culling, we render directly to G-buffer, otherwise
    // there is an extra blitting pass to normalize cumulated values.
    GFragment fragment;
    initGFragment(fragment);
    fragment.baseColor = out_color.rgb;
    fragment.normal = out_normal.xyz;
    fragment.ws_coord = inData.position_ws;
    fragment.material_id = pbrMaterial;
    autoPackGFragment(fragment);
#endif // not SHELL_CULLING
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE
