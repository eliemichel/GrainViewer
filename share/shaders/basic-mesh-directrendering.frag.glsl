#version 450 core
#include "sys:defines"

in vec3 position_ws;
in vec3 normal_ws;
in vec3 normal_cs;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;

layout (location = 0) out vec4 out_color;

struct Light {
    vec3 position_ws;
    vec3 color;
};

uniform Light light[1] = Light[1](Light(vec3(2.0, 2.0, 2.0), vec3(1.0)));

uniform vec4 baseColor = vec4(1.0, 0.5, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 0.0;
uniform float roughness = 0.7;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 1.0;

#include "include/uniform/camera.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/bsdf.inc.glsl"

void main() {
	SurfaceAttributes surface;
	surface.baseColor = baseColor.rgb;
	surface.metallic = metallic;
	surface.roughness = roughness;
	surface.reflectance = 0.5;

    out_color = vec4(0.0, 0.0, 0.0, 1.0);

    uint k = 0;
	vec3 camPos_ws = vec3(inverseViewMatrix[3]);
	vec3 toCam = normalize(camPos_ws - position_ws);
	vec3 toLight = normalize(light[k].position_ws - position_ws);
	vec3 f = brdf(toCam, normal_ws, toLight, surface);
	out_color.rgb += f * light[k].color;
}
