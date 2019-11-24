#version 450 core
#include "sys:defines"

in vec3 position_ws;
in vec3 normal_ws;
in vec3 normal_cs;
in vec3 tangent_ws;
in vec2 uv_ts;
flat in uint matId;

layout (location = 0) out vec4 color;

uniform vec4 baseColor = vec4(1.0, 0.5, 0.0, 1.0);
uniform float height = 0.0;
uniform float metallic = 1.0;
uniform float roughness = 0.5;
uniform float occlusion = 1.0;
uniform vec3 emission = vec3(0.0, 0.0, 0.0);
uniform vec3 normal = vec3(0.5, 0.5, 1.0);
uniform float normal_mapping = 0.0;

uniform vec3 light0_position = vec3(2.0, 2.0, 3.0) * 5;
uniform vec3 light1_position = vec3(1.0, -2.0, 2.0) * 5;
uniform vec3 light2_position = vec3(-1.0, 2.0, -1.0) * 5;

#include "include/uniform/camera.inc.glsl"

#include "include/utils.inc.glsl"

#define BURLEY_DIFFUSE
//#define ANISOTROPIC
#include "include/bsdf.inc.glsl"

void main() {
    vec3 bitangent_wsn = normalize(cross(normal_ws, tangent_ws));
    vec3 tangent_wsn = normalize(tangent_ws);
    vec3 normal_wsn = normalize(normal_ws);
	mat3 TBN = mat3(
        bitangent_wsn,
        tangent_wsn,
        normal_wsn
    );
    normal_wsn = normalize(normal_wsn + TBN * (normal * 2.0 - 1.0) * normal_mapping);

    vec3 c = baseColor.rgb * vec3(0.1, 0.15, 0.2);
    vec3 toLight;

    vec3 toCamera = -normalize(position_ws - cameraPosition(viewMatrix));
    
    SurfaceAttributes surface;
    surface.baseColor = vec3(1.0, 0.0, 0.0);
    surface.metallic = 0.0;
    surface.roughness = 0.5;
    surface.reflectance = 1.0;
    surface.emissive = vec3(0.0, 0.0, 0.0);
    surface.occlusion = 1.0;
#ifdef ANISOTROPIC
    surface.anisotropy = -0.5;

    bitangent_wsn = normalize(cross(normal_wsn, tangent_wsn));
    tangent_wsn = normalize(cross(bitangent_wsn, normal_wsn));
#endif

    toLight = -normalize(position_ws - light0_position);
#ifdef ANISOTROPIC
    c += brdf(toCamera, normal_wsn, toLight, tangent_wsn, bitangent_wsn, surface);
#else
    c += brdf(toCamera, normal_wsn, toLight, surface);
#endif
    
    toLight = -normalize(position_ws - light1_position);
    c += brdf(toCamera, normal_wsn, toLight, surface);
    toLight = -normalize(position_ws - light2_position);
    c += brdf(toCamera, normal_wsn, toLight, surface);

	color = vec4(c, 1.0);
}
