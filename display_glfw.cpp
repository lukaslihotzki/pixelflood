#include "display_glfw.hpp"
#include <stdexcept>
#include <GLFW/glfw3.h>

void displayCanvas(Canvas& canvas)
{
	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("glfwInit failed!");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	GLFWwindow* window = glfwCreateWindow(canvas.width, canvas.height, "pixelflood", nullptr, nullptr);

	if (!window) {
		throw std::runtime_error("glfwCreateWindow failed!");
	}

	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, [] (GLFWwindow*, int width, int height) {
		glViewport(0, 0, width, height);
	});

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	float vertices[][4] = {{-1.f,-1.f,+0.f,+1.f},
	                       {+1.f,-1.f,+1.f,+1.f},
	                       {-1.f,+1.f,+0.f,+0.f},
	                       {+1.f,+1.f,+1.f,+0.f}};
	glVertexPointer(2, GL_FLOAT, sizeof(vertices[0]), &(vertices[0][0]));
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertices[0]), &(vertices[0][2]));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	do {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, canvas.data);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertices) / sizeof(vertices[0]));
		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));

	glfwDestroyWindow(window);
}
