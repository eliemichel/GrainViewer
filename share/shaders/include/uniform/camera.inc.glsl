
layout (std140) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    mat4 inverseViewMatrix;
    vec2 resolution;
    vec2 _pad_8136548;
};
