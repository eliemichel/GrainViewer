#version 450 core
#include "sys:defines"

#pragma variant STAGE_EPSILON_ZBUFFER STAGE_BLIT_TO_MAIN_FBO STAGE_EXTRA_INIT
#pragma variant NO_DISCARD_IN_EPSILON_ZBUFFER
#pragma variant NO_COLOR_OUTPUT // used when there is an EXTRA_INIT pass then
#pragma variant USING_ShellCullingFragDepth // to use with FragDepth ShellCullingStrategy
#pragma variant USING_ExtraFbo // to enable in all shaders when ExtraFbo option is used

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

uniform bool uCheckboardSprites;
uniform bool uShowSampleCount;

#include "include/uniform/camera.inc.glsl"

#include "include/zbuffer.inc.glsl"

// Compute influence weight of a grain fragment
// sqDistToCenter = dot(uv, uv);
float computeWeight(vec2 uv, float sqDistToCenter) {
    switch (uWeightMode) {
    case cWeightLinear:
        if (uDebugShape == cDebugShapeSquare) {
            return 1.0 - max(abs(uv.x), abs(uv.y));
        } else {
            return 1.0 - sqrt(sqDistToCenter);
        }
    case cWeightQuad:
        if (uDebugShape == cDebugShapeSquare) {
            float d = max(abs(uv.x), abs(uv.y));
            return 1.0 - d * d;
        } else {
            return 1.0 - sqDistToCenter;
        }
    case cWeightGaussian:
        return -1.0; // TODO
    case cWeightNone:
    default:
        return 1.0;
    }
}

///////////////////////////////////////////////////////////////////////////////
#if defined(STAGE_BLIT_TO_MAIN_FBO)

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
    vec3 cs_coord = (linearizeDepth(d) - uEpsilon) * ray_cs.direction;
    fragment.ws_coord = (inverseViewMatrix * vec4(cs_coord, 1.0)).xyz;

    if (uShowSampleCount) {
        fragment.material_id = colormapDebugMaterial;
        fragment.baseColor = vec3(in_color.a);
    }

    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(STAGE_EPSILON_ZBUFFER)

layout (location = 0) out vec4 color0;

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
    color0 = vec4(0.0);
#endif // NO_COLOR_OUTPUT
}

///////////////////////////////////////////////////////////////////////////////
#elif defined(STAGE_EXTRA_INIT) // STAGE_EXTRA_INIT

layout (location = 0) out vec4 color0;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }

    // Init for accumulation
    color0 = vec4(0.0);
}


///////////////////////////////////////////////////////////////////////////////
#elif defined(USING_ExtraFbo)
// STAGE_DEFAULT when using Extra FBO does not write to a GFragment but to some
// ad-hoc attachment layout.

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;

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

uniform sampler2D uDepthTexture;
uniform float uTime;

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
    vec2 correctedPointCoord = (ceil(gl_PointCoord.xy * inData.screenSpaceDiameter) - 0.5) / inData.screenSpaceDiameter;
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float sqDistToCenter = dot(uv, uv);

    float weight = computeWeight(uv, sqDistToCenter);

    if (uDebugShape != cDebugShapeSquare && sqDistToCenter > 1.0) {
        //discard;
    }
    float antialiasing = 1.0;
    if (uDebugShape != cDebugShapeSquare) {
        antialiasing = smoothstep(1.0, 0.8, sqDistToCenter);
    }
    weight *= antialiasing;

    vec3 n;
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
    if (uDebugShape == cDebugShapeLitSphere || uDebugShape == cDebugShapeNormalSphere) {
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

    out_color.rgb = inData.baseColor.rgb;
    out_color.a = 1.0;
    out_normal.xyz = n;

    // direct shading
    if (uDebugShape != -1) {
        vec3 beauty = vec3(0.0, 0.0, 0.0);

        if (uDebugShape == cDebugShapeNormalSphere) {
            beauty.rgb = normalize(n) * 0.5 + 0.5;
        } else {
            beauty = inData.baseColor;
        }

        // DEBUG checkerboard for mipmap test
        if (uCheckboardSprites) {
            beauty = vec3(abs(step(uv.x, 0.0) - step(uv.y, 0.0)));
        }

        // TODO: move to computeWeight
        if (uShellDepthFalloff) {
            float d = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
            float limitDepth = linearizeDepth(d);
            float depth = linearizeDepth(gl_FragCoord.z);
            float fac = (limitDepth - depth) / uEpsilon;
            //beauty = vec3(1.0, fac, 0.0);
            weight *= fac;
        }

        out_color.rgb = beauty * weight;
        out_color.a = weight;
        out_normal.xyz = n * weight;
    }

    if (uShowSampleCount) {
        out_color.a = clamp(ceil(antialiasing), 0, 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
#else // STAGE_DEFAULT

#define OUT_GBUFFER
#include "include/gbuffer2.inc.glsl"

in FragmentData {
    vec3 position_ws;
    vec3 baseColor;
    float radius;
    float screenSpaceDiameter;
} inData;

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

    float weight = computeWeight(uv, sqDistToCenter);

    vec3 n;
    if (uDebugShape == cDebugShapeLitSphere || uDebugShape == cDebugShapeNormalSphere) {
        Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
        Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);
        vec3 sphereHitPosition_ws;
        bool intersectSphere = intersectRaySphere(sphereHitPosition_ws, ray_ws, inData.position_ws.xyz, inData.radius);
        if (intersectSphere) {
            n = normalize(sphereHitPosition_ws - inData.position_ws);
        } else {
            //discard;
            n = -ray_ws.direction;
        }
    } else {
        n = normalize(normal);
    }

    GFragment fragment;
    fragment.baseColor = inData.baseColor.rgb;
    fragment.normal = n;
    fragment.ws_coord = inData.position_ws;
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
            vec3 toCam = normalize(camPos_ws - inData.position_ws);

            SurfaceAttributes surface;
            surface.baseColor = fragment.baseColor;
            surface.metallic = fragment.metallic;
            surface.roughness = fragment.roughness;
            surface.reflectance = 0.7;

            for (int k = 0 ; k < 1 ; ++k) {
                vec3 toLight = normalize(vec3(5.0, 5.0, 5.0) - inData.position_ws);
                vec3 f = vec3(0.0);
                f = brdf(toCam, n, toLight, surface);
                beauty.rgb += f * vec3(3.0);
            }
        } else if (uDebugShape == cDebugShapeNormalSphere) {
            beauty.rgb = normalize(n) * 0.5 + 0.5;
        } else {
            beauty = inData.baseColor;
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

    autoPackGFragment(fragment);
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE
