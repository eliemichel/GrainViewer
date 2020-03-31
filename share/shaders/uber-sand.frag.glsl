#version 450 core
#include "sys:defines"

#pragma varopt STAGE_ZPASS STAGE_EPSILON_ZPASS STAGE_BLIT_TO_MAIN_FBO STAGE_EXTRA_INIT
#pragma option NO_DISCARD_IN_EPSILON_ZBUFFER

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
#elif defined(STAGE_EPSILON_ZPASS)

layout (location = 0) out vec4 color0;

void main() {
#ifndef NO_DISCARD_IN_EPSILON_ZBUFFER
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    if (uDebugShape != cDebugShapeSquare && dot(uv, uv) > 1.0) {
        discard;
    }
#endif // NO_DISCARD_IN_EPSILON_ZBUFFER
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
#else // DEFAULT STAGE

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

    out_color.rgb = inData.baseColor.rgb;
    out_color.a = 1.0;
    out_normal.xyz = n;

    // direct shading
    if (uDebugShape != -1) {
        vec3 baseColor = inData.baseColor;

        // DEBUG checkerboard for mipmap test
        if (uCheckboardSprites) {
            baseColor = vec3(abs(step(uv.x, 0.0) - step(uv.y, 0.0)));
        }

        // TODO: move to computeWeight
        if (uShellDepthFalloff) {
            float d = texelFetch(uDepthTexture, ivec2(gl_FragCoord.xy), 0).x;
            float limitDepth = linearizeDepth(d);
            float depth = linearizeDepth(gl_FragCoord.z);
            float fac = (limitDepth - depth) / uEpsilon;
            //baseColor = vec3(1.0, fac, 0.0);
            weight *= fac;
        }

        out_color.rgb = baseColor * weight;
        out_color.a = weight;
        out_normal.xyz = n * weight;
    }

    if (uShowSampleCount) {
        out_color.a = clamp(ceil(antialiasing), 0, 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
#endif // STAGE
