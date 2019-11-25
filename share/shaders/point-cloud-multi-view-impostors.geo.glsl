#version 450 core

#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
uniform mat4 invViewMatrix;
#include "include/uniform/camera.inc.glsl"
uniform float time;
uniform float focalLength;
uniform vec2 iResolution;

uniform float radius = 0.05;

flat in int vId[];
flat in vec3 vColor[];

out vec3 fColor;
out vec4 position_cs;
out vec4 position_ws;
flat out int id;
out mat4 gs_from_ws;
out mat4 ws_from_gs;
flat out float actualRadius;

#include "include/utils.inc.glsl"
#include "include/random.inc.glsl"


mat3 randomGrainOrientation() {
    vec3 vx = normalize(randVec(vec3(id, id, id)));
    vec3 vy = normalize(cross(vec3(0,0,1), vx));
    vec3 vz = normalize(cross(vx, vy));
    float t = 0.0;
    return mat3(vx, vy * cos(t) - vz * sin(t), vy * sin(t) + vz * cos(t));
}


// TODO: put grain position in grain matrix
mat4 randomGrainMatrix(vec4 position_ws = vec4(0.0, 0.0, 0.0, 1.0)) {
    mat3 rot = randomGrainOrientation();
    return mat4(
        vec4(rot[0], 0.0),
        vec4(rot[1], 0.0),
        vec4(rot[2], 0.0),
        position_ws
    );
}


void main() {
    id = vId[0];
    //actualRadius = mix(radius * 0.7, radius * 1.3, randv2(vec2(id, id*2306.87)));
    actualRadius = 0.018;

    position_cs = viewModelMatrix * gl_in[0].gl_Position;
    gl_Position = projectionMatrix * position_cs;
    gl_PointSize = 1.0 * max(iResolution.x, iResolution.y) * projectionMatrix[1][1] * actualRadius / gl_Position.w;
    if (isOrthographic(projectionMatrix)) {
        float a = projectionMatrix[0][0] * iResolution.x;
        float c = projectionMatrix[1][1] * iResolution.y;
        gl_PointSize = max(a * actualRadius, c * actualRadius);  // DEBUG
    }
    fColor = vColor[0];

    mat4 cs_from_ws = viewMatrix;
    mat4 ws_from_cs = invViewMatrix;
    position_ws = ws_from_cs * position_cs;
    // TODO: bake on CPU
    gs_from_ws = randomGrainMatrix(position_ws);
    ws_from_gs = inverse(gs_from_ws);

    gl_PointSize = 3.0;

    EmitVertex();
    EndPrimitive();
}
