//////////////////////////////////////////////////////
// Microfacet distributions and utility functions
// Largely from https://google.github.io/filament/Filament.html
#pragma variant BURLEY_DIFFUSE, FAST_GGX, ANISOTROPIC, UNCORRELATED_GGX

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

    a2 = at * ab;
    return NoH / (PI * NoH*NoH*NoH*NoH * 4);
}

float D_Beckmann(float NoH, const vec3 h, const vec3 t, const vec3 b, float at, float ab) {
    float ToH = dot(t, h);
    float BoH = dot(b, h);
    float a2 = at * ab;
    highp vec3 v = vec3(ab * ToH, at * BoH, a2 * NoH);
    highp float v2 = dot(v, v);
    float w2 = a2 / v2;
    return a2 * w2 * w2 * (1.0 / PI);
}

float V_SmithGGX(float NoV, float NoL, float roughness) {
    float a = roughness;
    float GGXL = NoL / (NoL * (1.0 - a) + a);
    float GGXV = NoV / (NoV * (1.0 - a) + a);
    return GGXL * GGXV;
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

float DistributionGGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = pi * denom * denom;
    
    return a2 / denom;
}


//#define DUPUY
#ifdef DUPUY

vec3 fresnel(float cos_theta_d, const SurfaceAttributes surface)
{
    vec3 f0 = 0.16 * surface.reflectance * surface.reflectance * (1.0 - surface.metallic) + surface.baseColor * surface.metallic;
    return F_Schlick(cos_theta_d, f0);
}

float p22(float xslope, float yslope, const SurfaceAttributes surface)
{
    float xslope_sqr = xslope * xslope;
    float yslope_sqr = yslope * yslope;
    return exp(-(xslope_sqr + yslope_sqr)) / PI;
}

float ndf(const vec3 h, const vec3 n, const SurfaceAttributes surface)
{
    vec2 alpha = vec2(1.0);
    float rho = 0.0;
    float rho2 = rho * rho;

    float cos_theta_h_sqr = h.z * h.z;
    float cos_theta_h_sqr_sqr = cos_theta_h_sqr * cos_theta_h_sqr;
    float xslope = -h.x / h.z + n.x / n.z;
    float yslope = -h.y / h.z + n.y / n.z;

    float scale = 1.0 / (alpha.x * alpha.y * sqrt(1 - rho2));

    float anisotropic_xslope = xslope / alpha.x;
    float anisotropic_yslope = (alpha.x * yslope - rho * alpha.y * xslope) * scale;

    return p22(anisotropic_xslope, anisotropic_yslope, surface) / cos_theta_h_sqr_sqr * scale;
}

float erf(float x)
{
    return 0.0; // todo
}

float g1_beckmann(const vec3 h, const vec3 k, const SurfaceAttributes surface)
{
    float v = 1.0 / tan(k.z);
    float tan_v = tan(v);
    float tan_v_sqr = tan_v * tan_v;
    return 2.0 / (1 + sqrt(1 + tan_v_sqr));
}

float g1_ggx(const vec3 h, const vec3 k, const SurfaceAttributes surface)
{
    float v = 1.0 / tan(k.z);
    float v2 = v * v;
    return 2.0 / (1 + erf(v) + exp(-v2) / (v * sqrt(PI)));
}

float g1(const vec3 h, const vec3 k, const vec3 n, const SurfaceAttributes surface)
{
    vec2 alpha = vec2(1.0);
    float rho = 0.0;
    float rho2 = rho * rho;

    float a = k.x * alpha.x + k.y * alpha.y * rho;
    float b = alpha.y * k.y * sqrt(1.0 - rho2);
    float c = k.z + k.x * n.x / n.z + k.y * n.y / n.z;
    vec3 kprime = normalize(vec3(a, b, c));

    return g1_ggx(h, k, surface);

    float g = g1_ggx(h, kprime, surface);
    if (g > 0.0) {
        return g * (k.z / c);
    } else {
        return 0.0;
    }
}

float gaf(const vec3 h, const vec3 i, const vec3 o, const vec3 n, const SurfaceAttributes surface)
{
    float G1_o = g1(h, o, n, surface);
    float G1_i = g1(h, i, n, surface);
    float tmp = G1_o * G1_i;
    if (tmp > 0.0) {
        return tmp / (G1_i + G1_o - tmp);
    } else {
        return 0.0;
    }
}

vec3 brdf(const vec3 v, vec3 n, const vec3 l, const SurfaceAttributes surface)
{
    // Convert to normal space
    vec3 o = mat3(viewMatrix) * v;
    vec3 i = mat3(viewMatrix) * l;
    n = mat3(viewMatrix) * n;

    vec3 h = normalize(i + o);
    float G = gaf(h, i, o, n, surface);

    if (G > 0.0) {
        float cos_theta_d = dot(o, h);
        vec3 F = fresnel(cos_theta_d, surface);
        float D = ndf(h, n, surface);
        return (F * D * G) / (4.0 * o.z * i.z);
    } else {
        return vec3(0.0);
    }
}

#else // DUPUY

#ifdef ANISOTROPIC
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, const vec3 t, const vec3 b, const SurfaceAttributes surface)
#else
vec3 brdf(const vec3 v, const vec3 n, const vec3 l, SurfaceAttributes surface)
#endif
{
    if (dot(l, n) < 0.0) {
        return vec3(0.0);
    }

    vec3 diffuseColor = (1.0 - surface.metallic) * surface.baseColor.rgb;
    vec3 f0 = 0.16 * surface.reflectance * surface.reflectance * (1.0 - surface.metallic) + surface.baseColor * surface.metallic;
    vec3 h = normalize(v + l);

    float NoV = abs(dot(n, v)) + 1e-5;
    float NoL = clamp(dot(n, l), 0.0, 1.0);
    float NoH = clamp(dot(n, h), 0.0, 1.0);
    float LoH = clamp(dot(l, h), 0.0, 1.0);
    float VoH = clamp(dot(v, h), 0.0, 1.0);

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
  #ifdef UNCORRELATED_GGX
    float V = V_SmithGGX(NoV, NoL, alpha);
  #else // UNCORRELATED_GGX
    #ifdef FAST_GGX
      float V = V_SmithGGXCorrelatedFast(NoV, NoL, alpha);
    #else
      float V = V_SmithGGXCorrelated(NoV, NoL, alpha);
    #endif
  #endif // UNCORRELATED_GGX
#endif

    vec3 F = F_Schlick(LoH, f0);
    vec3 Fs = (D * V) * F;
#ifdef BURLEY_DIFFUSE
    vec3 Fd = Fd_Burley(NoV, NoL, LoH, alpha) * diffuseColor;
#else
    vec3 Fd = Fd_Lambert() * diffuseColor;
#endif

    return (Fs + Fd) * NoL;
}

#endif // DUPUY
