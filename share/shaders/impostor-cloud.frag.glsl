#version 450 core
#include "sys:defines"

flat in int id;
in vec3 position_ws;

layout (location = 0) out vec4 gbuffer_color1;
layout (location = 1) out uvec4 gbuffer_color2;
layout (location = 2) out uvec4 gbuffer_color3;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"

#include "include/gbuffer.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/raytracing.inc.glsl"
#include "include/impostor.inc.glsl"

uniform SphericalImpostor impostor[3];

uniform float roughness = 0.5;
uniform float metallic = 0.0;

void main() {
	vec3 baseColor = vec3(float((id/255/255) % 255) / 255, float((id/255) % 255) / 255, float(id % 255) / 255);
	vec3 normal_ws = vec3(0.0, 0.0, 1.0);

	Ray ray_cs = fragmentRay(gl_FragCoord, projectionMatrix);

    Ray ray_ws = TransformRay(ray_cs, inverseViewMatrix);

	GFragment fragment = IntersectRaySphericalGBillboardNoInterp(impostor[0], ray_ws, position_ws, 0.01);
	//GFragment fragment = IntersectRaySphere(ray_ws, position_ws, 0.005);

	if (fragment.alpha < 0.5) discard;
	//GFragment fragment;
    //fragment.baseColor = baseColor;
    //fragment.normal = normal_ws;
    //fragment.ws_coord = position_ws;
    //fragment.material_id = pbrMaterial;
    //fragment.roughness = roughness;
    //fragment.metallic = metallic;
    //fragment.emission = vec3(0.0);
    packGFragment(fragment, gbuffer_color1, gbuffer_color2, gbuffer_color3);
}
