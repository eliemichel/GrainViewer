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

#ifndef H_WINDOWUTILS
#define H_WINDOWUTILS

#include <GLFW/glfw3.h>

/**
* Initialize GLFW, make it open a window, then load an opengl context in it
* using glad.
* This must not be called more than once. You must call terminateWindow() at
* the end of the program.
* @return the create window or a null pointer on error
*/
GLFWwindow * startup();

/**
* Destroys the window and terminate GLFW. Must be called at the end of the
* program when startup() has been called at the beginning.
*/
void shutdown(GLFWwindow *window);

#endif // H_WINDOWUTILS
