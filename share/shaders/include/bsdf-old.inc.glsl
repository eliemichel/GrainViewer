//////////////////////////////////////////////////////
// Microfacet distributions and utility functions

float DistributionGGX(vec3 N, vec3 H, float a) {
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = pi * denom * denom;
    
    return a2 / denom;
}


float GeometrySchlickGGX(float NdotV, float k) {
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}


float GeometrySmith(float NdotV, float NdotL, float k) {
    float ggx1 = GeometrySchlickGGX(NdotV, k);
    float ggx2 = GeometrySchlickGGX(NdotL, k);
    
    return ggx1 * ggx2;
}


vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


vec3 bsdf(in vec3 toCam, in vec3 toLight, in vec3 n, in vec3 albedo, in float Kd, in float Ks, in float a) {
    vec3 color = vec3(0.f);

    vec3 Wo = toCam;
    vec3 Wi = toLight;
    float NdotWo = max(dot(n, Wo), 0.0);
    float NdotWi = max(dot(n, Wi), 0.0);

    float Kdirect = (a + 1) * (a + 1) / 8;
    vec3 F0 = mix(vec3(0.04), albedo, a);

    float cosTheta = dot(n, Wi);
    if (cosTheta <= 0 || dot(n, Wo) <= 0) {
        return color;
    }
    vec3 h = normalize(Wo + Wi);
    vec3 L = vec3(3.0);
    vec3 F = fresnelSchlick(dot(n, Wo), F0);
    float D = DistributionGGX(n, h, a);
    float G = GeometrySmith(NdotWo, NdotWi, Kdirect);
    vec3 specular = D * F * G / (4.f * dot(n, Wo) * cosTheta);
    vec3 diffuse = albedo / pi;
    color += (Kd * diffuse + Ks * specular) * L * cosTheta;

    return color;
}


/*
vec3 bsdfPbr(in vec3 Wo, in vec3 n, in vec3 albedo, in float a) {
    float NdotWo = max(dot(n, Wo), 0.0);
    int minSteps = 400;
    vec3 sum = vec3(0.0f);
    float dW  = 1.0f / minSteps;
    float Kdirect = (a + 1) * (a + 1) / 8;
    float Ks = 1.f;
    float Kd = 1.f;
    vec3 F0 = mix(vec3(0.04), albedo, a);
    for(int i = 0, j = 0; i < maxSteps && j < minSteps; ++i) 
    {
        vec3 Wi = sphere[i];
        float cosTheta = dot(n, Wi);
        if (cosTheta <= 0) {
            // Cheaper to discard than to compute matrix product
            continue;
        }
        vec3 h = normalize(Wo + Wi);

        vec3 L = texture(cubemap, transpose(normalMatrix) * Wi).rgb;
        vec3 F = fresnelSchlick(NdotWo, F0);
        float D = DistributionGGX(n, h, a);
        float G = GeometrySmith(NdotWo, cosTheta, Kdirect);
        vec3 specular = D * F * G / (4.f * NdotWo * cosTheta);
        vec3 diffuse = albedo / pi;
        sum += (Kd * diffuse + Ks * specular) * L * cosTheta * dW;
        ++j;
    }
    return sum;
}
*/



vec3 bsdfPbrMetallicRoughness(in vec3 toCam, in vec3 toLight, in vec3 n, in vec3 baseColor, in float roughness, in float metallic) {
    vec3 h = normalize(toCam + toLight);
    float NdotWo = dot(n, toCam);
    float NdotWi = dot(n, toLight);

    float k = (roughness + 1) * (roughness + 1) / 8;
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);

    if (NdotWi <= 0 || NdotWo <= 0) {
        return vec3(0.0);
    }

    vec3 F = fresnelSchlick(NdotWo, F0);
    float D = DistributionGGX(n, h, roughness);
    float G = GeometrySmith(NdotWo, NdotWi, k);
    vec3 specular = baseColor * D * F * G / (4.f * NdotWo * NdotWi);
    vec3 diffuse = baseColor / pi;

    return (metallic * specular + (1.0 - metallic) * diffuse) * NdotWi;
}



/*
int invRoughnessLut(float roughness) {
    if (roughness <= 0.01f) {
        return 0;
    } else if (roughness <= 0.05f) {
        return 1;
    } else if (roughness <= 0.1f) {
        return 2;
    } else if (roughness <= 0.5f) {
        return 3;
    } else if (roughness <= 0.8f) {
        return 4;
    } else if (roughness <= 0.9f) {
        return 5;
    } else if (roughness <= 0.95f) {
        return 6;
    } else if (roughness <= 1.f) {
        return 7;
    } else {
        return 0;
    }
}
*/

int invRoughnessLut(float roughness) {
    if (roughness <= 0.01f) {
        return 0;
    } else if (roughness <= 0.1f) {
        return 1;
    } else if (roughness <= 0.8f) {
        return 2;
    } else {
        return 0;
    }
}


vec3 ApproximateSpecularIBL(vec3 specularColor , float roughness, vec3 N, vec3 V, sampler2D bsdfLut, samplerCubeArray filteredCubemaps) {
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    vec3 R = reflect(-V, N);
    float l = float(invRoughnessLut(roughness));
    vec3 prefilteredColor = texture(filteredCubemaps, vec4(R, l)).rgb;
    vec2 envBrdf = texture(bsdfLut, vec2(roughness, NoV)).rg;
    return prefilteredColor * (specularColor * envBrdf.x + envBrdf.y);
}

