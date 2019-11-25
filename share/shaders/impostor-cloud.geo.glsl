#version 450 core
#include "sys:defines"

layout (points) in;
layout (points, max_vertices = 1) out;

flat in int vId[];

flat out int id;

uniform mat4 modelMatrix;
uniform mat4 viewModelMatrix;
#include "include/uniform/camera.inc.glsl"
uniform float uInstanceLimit = 1.05;

#include "include/utils.inc.glsl"

void main() {
	id = vId[0];
	float actualRadius = 0.005;

    vec4 position_cs = viewModelMatrix * gl_in[0].gl_Position;

    if (length(position_cs) < uInstanceLimit) {
    	return;
    }

    gl_Position = projectionMatrix * position_cs;
    
    // Estimate projection of sphere on screen to determine sprite size
    gl_PointSize = 1.0 * max(resolution.x, resolution.y) * projectionMatrix[1][1] * actualRadius / gl_Position.w;
    if (isOrthographic(projectionMatrix)) {
        float a = projectionMatrix[0][0] * resolution.x;
        float c = projectionMatrix[1][1] * resolution.y;
        gl_PointSize = max(a * actualRadius, c * actualRadius);  // DEBUG
    }

    EmitVertex();
    EndPrimitive();
}
