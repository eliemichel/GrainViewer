
layout (std140) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 inverseViewMatrix;
    vec2 resolution;
    float uNear;
    float uFar;
    float uLeft;
    float uRight;
    float uTop;
    float uBottom;
};
