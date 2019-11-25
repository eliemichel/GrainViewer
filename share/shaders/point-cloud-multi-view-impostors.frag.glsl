#version 450 core

#include "sys:defines"

uniform samplerCube cubemap;
uniform sampler2DArray impostorTexture0;
uniform sampler2DArray impostorTexture1;
uniform sampler2DArray impostorTexture2;
uniform sampler2DArray impostorDepthTexture0;
uniform sampler2DArray impostorDepthTexture1;
uniform sampler2DArray impostorDepthTexture2;
uniform sampler2DArray impostorAlbedoTexture0;
uniform sampler2DArray impostorAlbedoTexture1;
uniform sampler2DArray impostorAlbedoTexture2;
uniform sampler2D colormapTexture;
uniform float time;
//uniform float radius = 0.015;
uniform float radius = 0.05;
uniform mat4 modelMatrix;
uniform mat4 invModelMatrix;
uniform mat4 viewMatrix;
uniform mat4 invViewMatrix;
uniform mat4 projectionMatrix;
uniform vec2 iResolution;

in vec3 fColor;
in vec4 position_cs;
in vec4 position_ws;
flat in int id;
in mat4 gs_from_ws;
in mat4 ws_from_gs;
flat in float actualRadius;

layout (location = 0) out vec4 out_color;
//layout (location = 0) out vec4 gbuffer_color1;
//layout (location = 1) out uvec4 gbuffer_color2;
//layout (location = 2) out uvec4 gbuffer_color3;
layout (depth_less) out float gl_FragDepth;

const float randomSeedBin = 1;

const int octaNb = 8;

#include "include/utils.inc.glsl"
#include "include/gbuffer2.inc.glsl"
#include "include/random.inc.glsl"
#include "include/bsdf.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/octahedron.inc.glsl"

// Matrix from billboard space to grain space
// ie view matrix that was used when rendering the billboard
mat4 bakingCameraMatrix(int billboardIndex, vec4 billboardPosition_gs) {
    vec3 roundedDirection_gs = viewIndexToDirection(billboardIndex, octaNb);

    // Axes x, y, z of the billboard basis relatively to the grain
    vec3 grainUp_gs = vec3(0.0, 0.0, 1.0);
    vec3 z = roundedDirection_gs;
    vec3 x = normalize(cross(grainUp_gs, z));
    vec3 y = normalize(cross(z, x));
    return mat4(
        vec4(x, 0.0),
        vec4(y, 0.0),
        vec4(z, 0.0),
        billboardPosition_gs
    );
}

struct BillboardIntersection {
    vec3 ts;  // Texture coordinates
    vec3 gs;  // Intersection point in grain space
    vec4 color;
    vec4 albedo;
    float metallic;
};

BillboardIntersection mixBillboardIntersection(BillboardIntersection i1, BillboardIntersection i2, float theta) {
    return BillboardIntersection(
        mix(i1.ts, i2.ts, theta),
        mix(i1.gs, i2.gs, theta),
        mix(i1.color, i2.color, theta),
        mix(i1.albedo, i2.albedo, theta),
        mix(i1.metallic, i2.metallic, theta)
    );
}

BillboardIntersection intersectRayBillboard(Ray ray_gs, vec4 billboardPosition_gs, float radius, int billboardIndex, sampler2DArray depthTexture, sampler2DArray impostorTexture, sampler2DArray albedoTexture) {
    mat4 gs_from_bs = bakingCameraMatrix(billboardIndex, billboardPosition_gs);
    mat4 bs_from_gs = inverse(gs_from_bs);

    Ray ray_bs = Ray(
        vec3(bs_from_gs * vec4(ray_gs.origin, 1.0)),
        mat3(bs_from_gs) * ray_gs.direction
    );

    // Intersection 
    vec3 uv_bs = intersectRayPlane(ray_bs, vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0));
    vec2 uv_ts = uv_bs.xy / radius * 0.5 + vec2(0.5);
    uv_ts.y = 1.0 - uv_ts.y;

    BillboardIntersection intersection;
    intersection.ts = vec3(uv_ts, billboardIndex);

    float depth = texture(depthTexture, intersection.ts).r;
    vec3 intersection_bs = uv_bs + vec3(0.0, 0.0, depth);
    intersection.gs = vec3(gs_from_bs * vec4(intersection_bs, 1.0));

    intersection.color = texture(impostorTexture, intersection.ts);
    intersection.albedo = texture(albedoTexture, intersection.ts);
    intersection.metallic = texture(depthTexture, intersection.ts).r;

    return intersection;
}

BillboardIntersection intersectRayMultiViewBillboard(Ray ray_gs, vec4 billboardPosition_gs, float radius, sampler2DArray depthTexture, sampler2DArray impostorTexture, sampler2DArray albedoTexture) {
    int billboardIndex = directionToViewIndex(-ray_gs.direction, octaNb);
    return intersectRayBillboard(ray_gs, billboardPosition_gs, radius, billboardIndex, depthTexture, impostorTexture, albedoTexture);
}

BillboardIntersection intersectRayMultiViewBillboardInterpolated(Ray ray_gs, vec4 billboardPosition_gs, float radius, sampler2DArray depthTexture, sampler2DArray impostorTexture, sampler2DArray albedoTexture) {
    int idx1, idx2, idx3, idx4;
    float du, dv;
    directionToViewIndexAll(-ray_gs.direction, idx1, idx2, idx3, idx4, du, dv, octaNb);

    float lambda = (1 - cos(du * pi)) * 0.5;
    float mu = (1 - cos(dv * pi)) * 0.5;

    BillboardIntersection intersection1 = intersectRayBillboard(ray_gs, billboardPosition_gs, radius, idx1, depthTexture, impostorTexture, albedoTexture);
    BillboardIntersection intersection2 = intersectRayBillboard(ray_gs, billboardPosition_gs, radius, idx2, depthTexture, impostorTexture, albedoTexture);
    BillboardIntersection intersection3 = intersectRayBillboard(ray_gs, billboardPosition_gs, radius, idx3, depthTexture, impostorTexture, albedoTexture);
    BillboardIntersection intersection4 = intersectRayBillboard(ray_gs, billboardPosition_gs, radius, idx4, depthTexture, impostorTexture, albedoTexture);

    return mixBillboardIntersection(
        mixBillboardIntersection(
            intersection1,
            intersection2,
            du
        ),
        mixBillboardIntersection(
            intersection3,
            intersection4,
            du
        ),
        dv
    );
}

BillboardIntersection intersectRayMultiViewBillboardMaybeInterpolated(Ray ray_gs, vec4 billboardPosition_gs, float radius, sampler2DArray depthTexture, sampler2DArray impostorTexture, sampler2DArray albedoTexture) {
#ifdef NO_INTERPOLATION
    return intersectRayMultiViewBillboard(ray_gs, billboardPosition_gs, radius, depthTexture, impostorTexture, albedoTexture);
#else // NO_INTERPOLATION
    return intersectRayMultiViewBillboardInterpolated(ray_gs, billboardPosition_gs, radius, depthTexture, impostorTexture, albedoTexture);
#endif // NO_INTERPOLATION
}

//
// ws: World Space
// cs: Camera Space
// gs: Grain Space (ie. Model Space for a given grain)
// bs: BillboardSpace (in which the billboard is along the plane Z=0)
//     NB: There is one bs per billboard, so 128 distinct bs
// ts: Texture Space
//

void main() {
    Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);

    vec3 p = intersectRaySphere(ray_cs, vec3(position_cs), actualRadius);

#ifdef SHADOW_MAP
    return;
#endif

    float A = projectionMatrix[2].z;
    float B = projectionMatrix[3].z;
    float depth = -p.z;
    gl_FragDepth  = 0.5*(-A*depth + B) / depth + 0.5;

    mat4 cs_from_ws = viewMatrix;
    mat4 ws_from_cs = invViewMatrix;
    mat4 gs_from_cs = gs_from_ws * ws_from_cs;
    mat4 cs_from_gs = cs_from_ws * ws_from_gs;

    Ray ray_gs = Ray(
        vec3(gs_from_cs * vec4(ray_cs.origin, 1.0)),
        mat3(gs_from_cs) * ray_cs.direction
    );

    vec4 position_gs = gs_from_ws * ws_from_cs * position_cs;

    BillboardIntersection intersection;
    float s = rand(vec3(id));
    if (s < 0.33) {
        intersection = intersectRayMultiViewBillboardMaybeInterpolated(ray_gs, position_gs, actualRadius, impostorDepthTexture0, impostorTexture0, impostorAlbedoTexture0);
    } else if (s < 0.67) {
        intersection = intersectRayMultiViewBillboardMaybeInterpolated(ray_gs, position_gs, actualRadius, impostorDepthTexture1, impostorTexture1, impostorAlbedoTexture1);
    } else {
        intersection = intersectRayMultiViewBillboardMaybeInterpolated(ray_gs, position_gs, actualRadius, impostorDepthTexture2, impostorTexture2, impostorAlbedoTexture2);
    }

    vec4 gcolor = intersection.color;

    if (gcolor.a <= 0.8) {
        discard;
    }

    depth = -(cs_from_gs * vec4(intersection.gs, 1.0)).z;
    //gl_FragDepth  = 0.5*(-A*depth + B) / depth + 0.5;

    vec3 normal_ws = mat3(ws_from_gs) * normalize(gcolor.xyz * 2.0 - vec3(1.0));

    /*
    vec3 reflectDir = reflect(-viewDirection_ws, normal_ws);
    vec3 c = vec3(texture(cubemap, reflectDir));
    vec4 reflectedColor = vec4(c, gcolor.a);

    float ior = 1.0 / 1.7;
    vec3 refractDir = refract(-viewDirection_ws, normal_ws, ior);
    c = vec3(texture(cubemap, refractDir));
    vec4 refractedColor = vec4(c, gcolor.a);
    */

    float fac = randv2(vec2(id, id));
    //*
    vec4 grainAlbedo = texture(colormapTexture, vec2(fac, 0.0));
    /*/
    vec4 grainAlbedo = intersection.albedo;
    if (s < 0.33) {
        //grainAlbedo.rgb = vec3(.5, .15,.1);
        //grainAlbedo.rgb = vec3(.5, .15,.1) * mix(.5, 1., s);
        //grainAlbedo.rgb = mix(vec3(.5, .15,.1) * .5, vec3(.4, .15,.15), s * 3.0);
    }
    //*/

    float roughness = 0.01;
    float metallic = intersection.metallic > 0.5 ? 1.0 : 0.0;
    metallic = 0.0;
    /*if (fac < 0.1) {
        roughness = 0.3;
        metallic = 1.0;
    }*/

    vec3 pos_ws = vec3(invModelMatrix * invViewMatrix * vec4(p, 1.0));
    
    GFragment fragment;
    fragment.baseColor = grainAlbedo.rgb;
    //fragment.baseColor = fColor.rgb;
    fragment.normal = normal_ws;
    fragment.ws_coord = pos_ws;
    fragment.material_id = pbrMaterial;
    //fragment.material_id = forwardBaseColorMaterial;
    //fragment.material_id = iblMaterial;
    fragment.roughness = roughness;
    fragment.metallic = metallic;
    fragment.emission = vec3(0.0);
    //packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
    out_color.rgb = fragment.normal * .5 + .5;

    out_color = vec4(1.0);
}
