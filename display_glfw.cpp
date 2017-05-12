#include "display_glfw.hpp"

#ifdef USE_GLEW
#include <GL/glew.h>
#endif

#include <stdexcept>
#include <GLFW/glfw3.h>

static const float vertices[][4] = {{-1.f,-1.f,+0.f,+1.f},
                                    {+1.f,-1.f,+1.f,+1.f},
                                    {-1.f,+1.f,+0.f,+0.f},
                                    {+1.f,+1.f,+1.f,+0.f}};

Display::Display(int width, int height)
{
	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("glfwInit failed!");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	window = glfwCreateWindow(width, height, "pixelflood", nullptr, nullptr);

	if (!window) {
		throw std::runtime_error("glfwCreateWindow failed!");
	}

	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, [] (GLFWwindow*, int width, int height) {
		glViewport(0, 0, width, height);
	});

	canvas.width = width;
	canvas.height = height;

	canvas.data = nullptr;

#ifdef USE_GLEW
	glewInit();
	if (GL_ARB_buffer_storage) {
		GLuint buf;
		glGenBuffers(1, &buf);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf);
		GLuint size = width * height * sizeof(uint32_t);
		GLuint flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, nullptr, flags | GL_CLIENT_STORAGE_BIT);
		canvas.data = (uint32_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size, flags);
		texImageBuf = nullptr;
	} else
#endif
		texImageBuf = canvas.data = new uint32_t[width * height];

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glEnable(GL_TEXTURE_2D);

	glVertexPointer(2, GL_FLOAT, sizeof(vertices[0]), &(vertices[0][0]));
	glTexCoordPointer(2, GL_FLOAT, sizeof(vertices[0]), &(vertices[0][2]));
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

Display::~Display()
{
	glfwDestroyWindow(window);

	if (texImageBuf) {
		delete texImageBuf;
	}
}

void Display::operator()()
{
	do {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImageBuf);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertices) / sizeof(vertices[0]));
		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));
}
