#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 color;

in vec2 uv;

uniform sampler2D uMainTexture;

uniform float uRadius;
uniform vec3 uCenter;
uniform float uFocalLength;
uniform vec2 uResolution;

uniform mat4 uViewMatrix;
uniform mat4 uProjectionMatrix;

void main() {
	color = texture(uMainTexture, uv);

	vec4 center_cs = uViewMatrix * vec4(uCenter, 1.0);
	float radius_cs = uRadius;
	float r2 = radius_cs * radius_cs;
	float ox = center_cs.x;
	float oy = center_cs.y;
	float oz = center_cs.z;
	float ox2 = ox * ox;
	float oy2 = oy * oy;
	float oz2 = oz * oz;
	float fl = uFocalLength;

	float x = -(uv.x * 1.0 - 0.5) * uResolution.x / uResolution.y;
	float y = -(uv.y * 1.0 - 0.5);

	// Ellipsis parameters
	float a = r2 - oy2 - oz2;
	float b = r2 - ox2 - oz2;
	float c = 2*ox*oy;
	float d = 2*ox*oz*fl;
	float e = 2*oy*oz*fl;
	float f = (r2 - ox2 - oy2) * fl * fl;
	float distance = a*x*x + b*y*y + c*x*y + d*x + e*y + f;

	if (distance < 0) {
		color.b += 0.1;
	} else {
		color.r -= 0.1;
	}

	float distance2;

	vec2 center = vec2(ox, oy) * oz * fl / (oz2 - r2);
	float fp = fl * fl * r2 * (ox2 + oy2 + oz2 - r2) / (oz2 - r2);

	distance2 = (
		a * (x - center.x) * (x - center.x)
		+ b * (y - center.y) * (y - center.y)
		+ c * (x - center.x) * (y - center.y)
		+ fp
	);

	float outerRadius = sqrt(abs(fp / (r2 - oz2)));
	float innerRadius = sqrt(abs(fp / (r2 - ox2 - oy2 - oz2)));
	distance2 = length(vec2(x, y) - center) - outerRadius;

	if (distance2 < 0) {
		color.g += 0.1;
	} else {
		color.g -= 0.1;
	}

	color.a = 1.0;
}
