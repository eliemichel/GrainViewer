#version 450 core
#include "sys:defines"

/**
 * use this shader with renderAdditive option of sand renderer
 */

in float radius;
in vec3 position_ws;

layout (location = 0) out vec4 out_color;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"

void main() {
	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);
	Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);

	vec3 hitPosition;
	if (!intersectRaySphere(hitPosition, ray_ws, position_ws.xyz, radius)) {
		discard;
	}

	out_color = vec4(vec3(1.0/400.0), 1.0);
	//out_color = vec4(hitPosition * vec3(0.1, 1.0, 1.0) - vec3(-0.5, 2.0, 0.0), 1.0);
	//out_color = out_color * vec4(normalize(hitPosition - position_ws) * .5 + .5, 1.0);
}
