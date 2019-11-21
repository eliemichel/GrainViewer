/**
* This file is part of Augen Light
*
* Copyright (c) 2017 - 2018 -- Élie Michel <elie.michel@exppad.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the “Software”), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* The Software is provided “as is”, without warranty of any kind, express or
* implied, including but not limited to the warranties of merchantability,
* fitness for a particular purpose and non-infringement. In no event shall the
* authors or copyright holders be liable for any claim, damages or other
* liability, whether in an action of contract, tort or otherwise, arising
* from, out of or in connection with the software or the use or other dealings
* in the Software.
*/

#ifndef H_SHADERUTILS
#define H_SHADERUTILS

#include <glad/glad.h>

/**
 * Check that the last shader compilation went good, and if not display the
 * compilation error message.
 */
bool checkShader(GLuint shader, const char *name);

/**
 * Check that the last program linking went good, and if not display the
 * linking error message.
 */
bool checkProgram(GLuint program);

#endif // H_SHADERUTILS
