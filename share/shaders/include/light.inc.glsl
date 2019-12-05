//////////////////////////////////////////////////////
// Light related functions

struct PointLight {
	sampler2D shadowMap;
	sampler2D richShadowMap;
	vec3 position_ws;
	vec3 color;
	int isRich;
	int hasShadowMap;
	mat4 matrix;
};


float shadowBiasFromNormal(const in PointLight light, const in vec3 normal) {
	if (light.isRich == 1) {
		return 0.0;
	} else {
		return max(0.00005 * (1.0 - dot(normal, vec3(1.0, 0.0, 0.0))), 0.00001);
	}
}


vec4 richLightTest(const in PointLight light, vec3 position_ws, vec3 position_cs, float shadowBias) {
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w * 0.5 + 0.5;

	float shadowLimitDepth = texture(light.shadowMap, shadowCoord.xy).r;
	//shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;

	vec2 s = vec2(textureSize(light.richShadowMap, 0));
	vec2 roundedShadowCoord = round(shadowCoord.xy * s) / s;

	vec3 shadowLimitTexelCenter = vec3(roundedShadowCoord, shadowLimitDepth);

	vec3 normal = normalize(texture(light.richShadowMap, shadowCoord.xy).xyz);
	if (normal.z > 0) {
		//normal = -normal;  // point toward camera
	}

	vec2 dp = (shadowCoord.xy - roundedShadowCoord.xy) * shadowCoord.w;
	// need to get the proj mat w/o the light view mat
	vec2 dv = (inverse(light.matrix) * vec4(dp, 0.0, shadowCoord.w)).xy;
	//vec2 grad = vec2(dFdx(position_cs.x), dFdy(position_cs.y));
	//vec2 dv = (shadowCoord.xy - roundedShadowCoord.xy) * grad;
	//return vec4(grad * 400.0, 0.0, 1.0);

	float d0 = texture(light.shadowMap, roundedShadowCoord).r;
	float d = d0 + dot(dv, normal.xy);

	//return vec4(normal.xy * 0.5 + 0.5, 0.0, 0.0);

	shadowBias = 0.0;
	return vec4((d - shadowCoord.z) * 1000.0 + 0.5);
	return vec4(d > shadowCoord.z + 0.00001 ? 1.0 : 0.0);

	//return vec4(dv * s.x + 0.5, 1.0);

	//return vec4(fac * 0.5 + 0.5);
	//return fac <= 0.01 ? 0.0 : 1.0;
}


float richShadowAt(const in PointLight light, vec3 position_ws, float shadowBias) {
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w * 0.5 + 0.5;

	float shadowLimitDepth = texture(light.shadowMap, shadowCoord.xy).r;
	//shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;

	vec2 s = vec2(textureSize(light.richShadowMap, 0));
	vec2 roundedShadowCoord = round(shadowCoord.xy * s) / s;

	vec3 shadowLimitTexelCenter = vec3(roundedShadowCoord, shadowLimitDepth);

	vec3 normal = normalize(texture(light.richShadowMap, shadowCoord.xy).xyz);
	if (normal.z > 0) {
		normal = -normal;  // point toward camera
	}

	float fac = dot(normalize(vec3(shadowCoord) - shadowLimitTexelCenter), normal);
	return fac;
	return fac <= 0.01 ? 0.0 : 1.0;
}


float shadowAt(const in PointLight light, vec3 position_ws, float shadowBias) {
	if (light.hasShadowMap == 0) {
		return 0.0;
	}

	if (light.isRich == 1) {
		return richShadowAt(light, position_ws, shadowBias);
	}

	float shadow = 0.0;
	vec4 shadowCoord = light.matrix * vec4(position_ws, 1.0);
	shadowCoord = shadowCoord / shadowCoord.w;
	if (abs(shadowCoord.x) >= 1.0 || abs(shadowCoord.y) >= 1.0) {
		return 1.0;
	}
	shadowCoord = shadowCoord * 0.5 + 0.5;

	// PCF
	vec2 texelSize = 1.0 / textureSize(light.shadowMap, 0);
	vec2 dcoord;
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			dcoord = vec2(x, y) * texelSize;
			float d = texture(light.shadowMap, shadowCoord.xy + dcoord).r;
			shadow += d < shadowCoord.z - shadowBias ? 1.0 : 0.0;
		}
	}

	return shadow / 9.0;
}
