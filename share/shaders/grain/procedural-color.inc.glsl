#include "random-grains.inc.glsl"

uniform sampler2D uColormapTexture;

vec3 proceduralColor(vec3 pos, uint pointId) {
	vec3 baseColor = vec3(0.0);

#if defined(PROCEDURAL_BASECOLOR)

    float r = randomGrainColorFactor(int(pointId));
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;

#elif defined(PROCEDURAL_BASECOLOR2)

    float r0 = randomGrainColorFactor(int(pointId));
    float r = pos.z*.6 + mix(0.35, 0.3, sin(pos.x*.5+.5))*r0;
    if (r0 < 0.5) {
        r = 1. - r0;
    }
    baseColor = texture(uColormapTexture, vec2(r, 0.0)).rgb;
    baseColor *= mix(vec3(0.9, 0.9, 0.9), vec3(1.6, 2.0, 2.0), r0);

#elif defined(PROCEDURAL_BASECOLOR3)

	float r = randomGrainColorFactor(int(pointId));
	float u = pow(0.5 + pos.z * 0.5, 3.0);
	if (r < 0.1) u = 0.01;
	if (r > 0.9) u = 0.99;
    baseColor = texture(uColormapTexture, vec2(clamp(u + (r - 0.5) * 0.2, 0.01, 0.99), 0.0)).rgb;

    // Add a blue dot
    float th = 0.01;
    if (abs(atan(pos.y, pos.x)) < th && abs(atan(pos.z, pos.x)) < th) {
	    baseColor = vec3(0.0, 0.2, 0.9);
	}

	baseColor = mix(baseColor, baseColor.bgr, smoothstep(0.75, 0.73, length(pos)));

#elif defined(PROCEDURAL_BASECOLOR_BLACK)

	baseColor = vec3(0.0, 0.0, 0.0);

#endif // PROCEDURAL_BASECOLOR

	return baseColor;
}

bool isUsingProceduralColor() {
#if defined(PROCEDURAL_BASECOLOR) || defined(PROCEDURAL_BASECOLOR2) || defined(PROCEDURAL_BASECOLOR3) || defined(PROCEDURAL_BASECOLOR_BLACK)
	return true;
#define USING_PRECEDURAL_COLOR
#else
	return false;
#endif // PROCEDURAL_BASECOLOR
}
