#version 450 core
#include "sys:defines"

#pragma varopt PASS_DEPTH PASS_EPSILON_DEPTH PASS_BLIT_TO_MAIN_FBO
#pragma opt SHELL_CULLING
#pragma opt NO_DISCARD_IN_PASS_EPSILON_DEPTH
#pragma opt PSEUDO_LEAN

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

uniform bool uDebugRenderType = false;
uniform vec3 uDebugRenderColor = vec3(32.0/255.0, 64.0/255.0, 161.0/255.0);

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

#ifdef PSEUDO_LEAN
#define IN_LEAN_LINEAR_GBUFFER
#else // PSEUDO_LEAN
#define IN_LINEAR_GBUFFER
#endif // PSEUDO_LEAN
#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

// depth attachement of the target fbo
// (roughly safe to use despite feedback loops because we know that there is
// exactly one fragment per pixel)
//uniform sampler2D uDepthTexture; // nope, depth test used as is, just write to gl_FragDepth

// attachments of the secondary fbo
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

    GFragment fragment;
    autoUnpackGFragment(fragment);

#ifdef NO_DISCARD_IN_PASS_EPSILON_DEPTH
    if (fragment.alpha < 0.00001) discard;
#endif // NO_DISCARD_IN_PASS_EPSILON_DEPTH
    float weightNormalization = 1.0 / fragment.alpha; // inverse sum of integration weights

    fragment.baseColor *= weightNormalization;
    fragment.normal *= weightNormalization;
    fragment.roughness *= weightNormalization;
    fragment.material_id = pbrMaterial;

#ifdef PSEUDO_LEAN
    // LEAN mapping
    fragment.lean1 *= weightNormalization; // first order moments
    fragment.lean2 *= weightNormalization; // second order moments
    vec2 B = fragment.lean1.xy;
    vec3 M = fragment.lean2.xyz;
    float s = 48.0; // phong exponent must be used but we use ggx :/
    M.xy += 1./s; // bake roughness into normal distribution
    vec2 diag = M.xy - B.xy * B.xy;
    float e = M.z - B.x * B.y;
    mat2 inv_sigma = inverse(mat2(diag.x, e, e, diag.y));

    fragment.normal = normalize(vec3(fragment.lean1.xy, 1));
    fragment.normal = mat3(inverseViewMatrix) * fragment.normal;
    fragment.roughness = length(diag);
#else // PSEUDO_LEAN
    float dev = clamp(pow(1. - length(fragment.normal), 0.2), 0, 1);
    fragment.roughness = mix(fragment.roughness, 1.0, dev);
#endif // PSEUDO_LEAN

    // Fix depth
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    vec3 cs_coord = (linearizeDepth(d) - uEpsilon) * ray_cs.direction;
    fragment.ws_coord = (inverseViewMatrix * vec4(cs_coord, 1.0)).xyz;

    //fragment.baseColor *= 0.8;

    if (uShowSampleCount) {
        fragment.material_id = colormapDebugMaterial;
        fragment.baseColor = vec3(fragment.alpha);
    }

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    }


    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(PASS_EPSILON_DEPTH)

void main() {
#ifndef NO_DISCARD_IN_PASS_EPSILON_DEPTH
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }
#endif // not NO_DISCARD_IN_PASS_EPSILON_DEPTH
}

///////////////////////////////////////////////////////////////////////////////
#else // DEFAULT PASS

// If not using shell culling, we render directly to G-buffer, otherwise
// there is an extra blitting pass to normalize cumulated values.
#ifdef SHELL_CULLING
#  ifdef PSEUDO_LEAN
#    define OUT_LEAN_LINEAR_GBUFFER
#  else // PSEUDO_LEAN
#    define OUT_LINEAR_GBUFFER
#  endif // PSEUDO_LEAN
#else // SHELL_CULLING
#  define OUT_GBUFFER
#endif // SHELL_CULLING
#include "include/gbuffer2.inc.glsl"

in FragmentData {
    vec3 position_ws;
    vec3 baseColor;
    float radius;
    float screenSpaceDiameter;
    float diameterOvershot;
    float metallic;
    float roughness;
} inData;

uniform float uTime;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
    GFragment fragment;
    initGFragment(fragment);

    vec2 uv = (gl_PointCoord * 2.0 - 1.0) * inData.diameterOvershot;
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

    fragment.baseColor.rgb = baseColor * weight;
    fragment.alpha = weight;
    fragment.normal = n * weight;
    fragment.metallic = inData.metallic * weight;
    fragment.roughness = inData.roughness * weight;

    // LEAN mapping
#ifdef PSEUDO_LEAN
    fragment.normal = mat3(viewMatrix) * fragment.normal;
    vec2 slopes = fragment.normal.xy / fragment.normal.z;
    if (fragment.normal.z == 0.0) {
        slopes = vec2(0.0);
    }
    fragment.lean1.xy = slopes.xy * weight;
    fragment.lean2.xy = slopes.xy * slopes.xy * weight;
    fragment.lean2.z = slopes.x * slopes.y * weight;
#endif // PSEUDO_LEAN

    if (uShowSampleCount) {
        fragment.alpha = clamp(ceil(antialiasing), 0, 1);
    }

#ifndef SHELL_CULLING
    fragment.ws_coord = inData.position_ws;
    fragment.material_id = pbrMaterial;

    if (uDebugRenderType) {
        fragment.baseColor = uDebugRenderColor;
    }
#endif // not SHELL_CULLING

    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE
