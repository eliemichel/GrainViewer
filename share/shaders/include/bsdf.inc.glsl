//////////////////////////////////////////////////////
// Microfacet distributions and utility functions
// Variant defines: BURLEY_DIFFUSE, FAST_GGX, ANISOTROPIC

struct SurfaceAttributes {
    vec3 baseColor;
    float metallic;
    float roughness;
    float reflectance;
    vec3 emissive;
    float occlusion;
#ifdef ANISOTROPIC
    float anisotropy;
#endif
};

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
float D_GGX(float roughness, float NoH, const vec3 n, const vec3 h) {
    vec3 NxH = cross(n, h);
    float a = NoH * roughness;
    float k = roughness / (dot(NxH, NxH) + a * a);
    float d = k * k * (1.0 / PI);
    return saturateMediump(d);
}

float D_GGX_Anisotropic(float NoH, const vec3 h, const vec3 t, const vec3 b, float at, float ab) {
    float ToH = dot(t, h);
    float BoH = dot(b, h);
    float a2 = at * ab;
    highp vec3 v = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / PI);
}

float V_SmithGGXCorrelated(float NoV, float NoL, float roughness) {
    float a2 = roughness * roughness;
    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelatedFast(float NoV, float NoL, float roughness) {
    float a = roughness;
    float GGXV = NoL * (NoV * (1.0 - a) + a);
    float GGXL = NoV * (NoL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL);
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV,
        float ToL, float BoL, float NoV, float NoL) {
    float lambdaV = NoL * length(vec3(at * ToV, ab * BoV, NoV));
    float lambdaL = NoV * length(vec3(at * ToL, ab * BoL, NoL));
    float v = 0.5 / (lambdaV + lambdaL);
    return saturateMediump(v);
}

vec3 F_Schlick(float LoH, vec3 f0) {
    float f = pow(1.0 - LoH, 5.0);
    return f + f0 * (1.0 - f);
}

float F_Schlick_Burley(float VoH, float f0, float f90) {
    return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float Fd_Lambert() {
    return 1 / PI;
}

float Fd_Burley(float NoV, float NoL, float LoH, float roughness) {
    float f90 = 0.5 + 2.0 * roughness * LoH * LoH;
    float lightScatter = F_Schlick_Burley(NoL, 1.0, f90);
    float viewScatter = F_Schlick_Burley(NoV, 1.0, f90);
    return lightScatter * viewScatter * (1.0 / PI);
}

#ifdef ANISOTROPIC
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, const vec3 t, const vec3 b, const SurfaceAttributes surface)
#else
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, SurfaceAttributes surface)
#endif
{
    vec3 diffuseColor = (1.0 - surface.metallic) * surface.baseColor.rgb;
    vec3 f0 = 0.16 * surface.reflectance * surface.reflectance * (1.0 - surface.metallic) + surface.baseColor * surface.metallic;
    vec3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);

    float alpha = surface.roughness * surface.roughness;

#ifdef ANISOTROPIC
    float ToV = clamp(dot(t, v), 0.0, 1.0);
    float BoV = clamp(dot(b, v), 0.0, 1.0);
    float ToL = clamp(dot(t, l), 0.0, 1.0);
    float BoL = clamp(dot(b, l), 0.0, 1.0);
    
    float at = max(alpha * (1.0 + surface.anisotropy), 0.001);
    float ab = max(alpha * (1.0 - surface.anisotropy), 0.001);
    float D = D_GGX_Anisotropic(NoH, h, t, b, at, ab);
    float V = V_SmithGGXCorrelated_Anisotropic(at, ab, ToV, BoV, ToL, BoL, NoV, NoL);
#else
    float D = D_GGX(alpha, NoH, n, h);
  #ifdef FAST_GGX
    float V = V_SmithGGXCorrelatedFast(NoV, NoL, alpha);
  #else
    float V = V_SmithGGXCorrelated(NoV, NoL, alpha);
  #endif
#endif

    vec3 F = F_Schlick(LoH, f0);
    vec3 Fs = (D * V) * F;
#ifdef BURLEY_DIFFUSE
    vec3 Fd = Fd_Burley(NoV, NoL, LoH, alpha) * diffuseColor;
#else
    vec3 Fd = Fd_Lambert() * diffuseColor;
#endif

    return Fs + Fd;
}

