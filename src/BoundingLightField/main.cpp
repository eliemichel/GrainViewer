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

#include <glad/glad.h> // must be the first include
#include "utils/window.h" // for startup() and shutdown()
#include "utils/shader.h" // for checkShader() and checkProgram()
#include <cstdlib> // for EXIT_FAILURE and EXIT_SUCCESS
#include <cmath> // for sin and cos

/**
 * Tip: place a breakpoint at line 96 in utils/debug.cpp
 */

/**
 * This is a demo file in which it is fine to use globals in order to minimize
 * boilerplate. If you would like to embed this in a more complexe application,
 * you would have make those globals the attributes of a class, and the
 * init/finish/update/render functions would be methods.
 */

/**
 * We first need a GPU buffer in which storing the vertex attributes, i.e.
 * everything we know about the vertex: position, color, etc.
 * We create for this a buffer object. It is a simple unsigned int (GLuint), an
 * identifier used internally by opengl and that refers to the actual buffer.
 * The buffer itself is allocated in the GPU memory, so it can only be affected
 * through OpenGL.
 * This buffer of vertex attributes is typically called a Vertex Buffer Object,
 * or VBO.
 */
GLuint vbo;

/**
 * Besides the buffer of attributes itself, we need to tell opengl how to
 * interprete it. This memory layout information is stored in an opengl object
 * called a Vertex Array Object.
 */
GLuint vao;

/**
 * Once we've got the data to render, we must tell how to draw it. This is done
 * with the shader program, that gathers at least a vertex shader and a
 * fragment shader. The program holds the user code that will run on the GPU.
 */
GLuint program;


void init() {
	//////////////////////////////////////////
	// 1. VERTEX BUFFER

	// This is our raw vertex data. It could be constructed dynamically but for
	// this example it is a simple array. The memory can be organized as one
	// wishes, we'll tell opengl how to interprete it later on.
	// Here we decided to list position0 color0 position1 color1 etc. We use
	// the type GLfloat because it will be sent to the GPU memory, that might
	// have a different definition of float than the CPU.
	const GLfloat points[] = {
		 0.f,  .5f,  0.f,   1.f, 0.f, 0.f,
		 .5f, -.5f,  0.f,   0.f, 1.f, 0.f,
		-.5f, -.5f,  0.f,   0.f, 0.f, 1.f,
	};

	// As any OpenGL object, it is first created using glCreate*() functions
	// and must be freed using similar glDelete*() functions.
	// NB: You'll find a lot of references to glGenBuffers. Here we use
	// glCreateBuffers which is new to OpenGL 4.5 and avoid in a lot of case to
	// use glBind* functions and allows to use function on so called "named
	// buffers" instead.
	glCreateBuffers(1, &vbo);

	// Allocate the memory on the GPU for the buffer named <vbo>. This requires
	// a size, and some flags about how to lay out the memory (it depends on
	// its usage).
	// The flag GL_DYNAMIC_STORAGE_BIT tells opengl that this buffer might be
	// modified from the CPU later. We set it in this example because the
	// update function gives an example of how to do so, but one could have
	// initialized an immutable buffer using:
	//     glNamedBufferStorage(vbo, sizeof(points), points, 0);
	// Checkout the documentation for more information:
	// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glBufferStorage.xhtml
	glNamedBufferStorage(vbo, sizeof(points), NULL, GL_DYNAMIC_STORAGE_BIT);

	// Sends data to the buffer. This copies data from <points>, which is
	// stored in the CPU RAM, to the buffer of <vbo>, which sits in the GPU
	// memory.
	glNamedBufferSubData(vbo, 0, sizeof(points), points);

	//////////////////////////////////////////
	// 2. VERTEX ARRAY

	// Create the Vertex Array Object
	glCreateVertexArrays(1, &vao);

	// Before doing anything on or with a VAO, we need to bind it to the
	// current context. There can be only one "active" VAO at a time. This is
	// very common mechanism in OpenGL to bind objects of a given role.
	glBindVertexArray(vao);

	// In this vertex array, we will list vertex ATTRIBUTES, representing for
	// instance the position, the color, etc. Let's start with the position,
	// that we'll place in attribute #0. We first have to enable it.
	glEnableVertexAttribArray(0);

	// We bind the VBO to the current context because the next function call
	// will refer to it, as stated in its documentation:
	// https://www.khronos.org/opengl/wiki/GLAPI/glVertexAttribPointer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	// Describe the first vertex attribute to OpenGL (of index #0). It is made
	// of 3 consecutive floats, that must not be normalized (this could be
	// useful for normals). The last two parameters are the stride and offset.
	// They tell how to get data from the buffer currently bound to
	// GL_ARRAY_BUFFER, namely our VBO.
	// The offset is the index of the first vertex position in the buffer.
	// Since our vertex buffer starts with a position (0.0, 0.5, 0.0), it is
	// set to 0.
	// The stride is the byte distance between two consecutive vertex position
	// informations. This is what allows us to mix both position and color in
	// the same buffer: we tell opengl to read 3 floats at position 0, then to
	// jump to 0 + 6 floats for the next position, thus skipping the 3 floats
	// that describe the vertex color.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	// The color vertex attribute follows the same pattern, and is the
	// attribute #1. The only difference is its offset in the VBO, because the
	// first color attribute is after the first position, so has an offset of
	// 3 floats. The stride remains the same because the next color is also
	// 6 floats away, at float 9.
	// No need to bind the VBO again, it is still bound.
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

	// We can unbind the VAO. This is not required, but it illustrate that
	// a. the VAO stores all the information about vertex attributes, and
	// b. unbinding objects in OpenGL is always done by binding the object 0
	glBindVertexArray(0);

	//////////////////////////////////////////
	// 3. SHADERS

	// Hardcoded shader source code. You may want to quickly switch to a
	// mechanism that loads them from files at run time.
	const char *vertexShaderSource =
		// Mandatory first line to specify the minimum target version of opengl
		// (450 for OpenGL 4.5)
		"#version 450\n"

		// Get the input attribute #0 (decribed in the VAO) as a vec3 and call
		// it "position"
		"layout(location=0) in vec3 vPosition;\n"
		// Get the input attribute #1 as a vec3 and call it "color"
		"layout(location=1) in vec3 vColor;\n"

		// Define a custom output attribute. It will be interpolated among faces
		// and provided to the fragment shader
		"out vec3 fColor;\n"

		"void main() {\n"
		// Fill the gl_Position output attribute, which is a vec4 required by
		// the fixed parts ("fixed" as in "non-programmable") of the GPU
		// pipeline
		"    gl_Position = vec4(vPosition, 1.0);\n"
		// Fill the fColor output attribute
		"    fColor = vColor;\n"
		"}\n";

	const char *fragmentShaderSource =
		"#version 450\n"

		// Get the attribute from the vertex shader, interpolated between the
		// values of the vertex of the triangle that generated this fragment.
		"in vec3 fColor;\n"

		// Implicitely `layout(location=0) out vec4 color`, this output will be
		// the value added to the color attachment 0 of the target framebuffer
		// (e.g. the screen)
		"out vec4 color;\n"

		// A variable that will be set from the CPU, see update()
		"uniform float uTime;"

		"void main() {\n"
		// Fill the output color
		"    float s = sin(uTime);\n"
		"    color = vec4(fColor * s * s, 1.0);"
		"}\n";

	// The program is created like any other opengl object:
	program = glCreateProgram();

	// We will load the GPU program by sections called the shaders.
	// As usual, a shader is handled through a GLuint.
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// We provide the source code to the shader object (the parameters are
	// complicated by the fact that it can take an array of C strings if
	// needed)
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	// As any source code, the shader must be compiled before it can be
	// executed. Note that the shaders are compiled at run time, when the
	// application is started, and not when this C++ program is compiled. This
	// is because the shader compilation depends on the user's GPU.
	glCompileShader(vertexShader);
	// Check for compilation errors and display message. Error handling should
	// be more advanced than that but its a minimum. (This is a custom function
	// of mine, defined in utils/shader.cpp.)
	checkShader(vertexShader, "vertexShader");
	// We add the compiled shader to the program
	glAttachShader(program, vertexShader);
	// The shader is only the text representation of the code, so now that it
	// is compiled and attached to a program, we can free its textual
	// representation.
	glDeleteShader(vertexShader);

	// Then same for the fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	checkShader(fragmentShader, "fragmentShader");
	glAttachShader(program, fragmentShader);
	glDeleteShader(fragmentShader);

	// Again, as for a normal program, its objects (the shaders, in this case)
	// must be linked together.
	glLinkProgram(program);
	checkProgram(program);

	//////////////////////////////////////////
	// 4. OTHER STUFF

	// Enable some opengl capabilities you need, e.g. depth buffering:
	glEnable(GL_DEPTH_TEST);
}

void finish() {
	glDeleteProgram(program);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

void update(double time) {
	// There are several ways to update a scene. The most obvious one is to
	// change values in its buffers.

	//////////////////////////////////////////
	// A. UPDATE VBO

	// As an example of live vertex buffer update, we move the third vertex
	// around its position.

	// Create the new vertex data
	GLfloat new_vertex[] = {
		-.5f, -.5f,  0.f,   0.f, 0.f, 1.f,
	};
	new_vertex[0] += static_cast<GLfloat>(.1 * cos(time));
	new_vertex[1] += static_cast<GLfloat>(.1 * sin(time));

	// Update the VBO, sending new data to the GPU memory.
	// The thrid point is located at an offset of twice the size of a vertex
	// data.
	GLintptr offset = 2 * sizeof(new_vertex);
	glNamedBufferSubData(vbo, offset, sizeof(new_vertex), new_vertex);

	//////////////////////////////////////////
	// B. UPDATE UNIFORMS

	// Uniforms are shader program global variables that are uniform for a
	// given draw call, but can be modified at any time between them.
	// Get the uniform index given its name. If the name does not match any
	// uniform, this returns -1. Beware that even if the uniform is declared,
	// it will be removed from the program at linkage if it is not used and so
	// will have no location.
	GLint loc = glGetUniformLocation(program, "uTime");
	if (loc != -1) {
		// Before using glUniform* we must bind the program
		glUseProgram(program);
		// There is a bunch of glUniform* functions to use depending on the
		// type of the uniform variable. Here we use glUniform1f to set a
		// single float.
		glUniform1f(loc, static_cast<GLfloat>(time));
	}
}

void render() {
	// Set the color that will be used to fill the screen
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Clear the framebuffer. This fills the color attachment with the color
	// defined on the previous line, but also reset the depth buffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Specify which program to run on the GPU
	glUseProgram(program);

	// Bind the VAO specifying where to get input data for the program. It
	// remembers the vertex attributes and the associated vertex buffer
	glBindVertexArray(vao);

	// Fire the actual drawing, after all!
	// The first parameter tells which rendering type is used, and parametrizes
	// the fixed rendering pipeline. The next numbers are the first vertex
	// index to render and the number of vertex.
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

/**
 * An example of key callback to feed to a GLFW window (see bellow in main)
 * /!\ key codes assume that you are using a Qwerty keyboard
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// Closes the application if the escape key is pressed
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}
}

int main(int argc, char *argv[]) {
	GLFWwindow *window = startup();
	if (!window) {
		return EXIT_FAILURE;
	}

	// This is an example of how to use GLFW event callbacks. Checkout the
	// documentation for other events:
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetKeyCallback(window, key_callback);

	init();

	while (!glfwWindowShouldClose(window)) {
		update(glfwGetTime());
		render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	finish();

	shutdown(window);
	return EXIT_SUCCESS;
}
