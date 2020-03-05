#pragma once

#include <glad/modernglad.h>
class GlBuffer;
class ShaderProgram;

/**
 * Input buffer is buffer0, output buffer is either buffer0 or buffer1, as
 * specified by the returned value.
 */
int prefixSum(const GlBuffer & buffer0, const GlBuffer & buffer1, const ShaderProgram & shader, GLuint pointCount);
