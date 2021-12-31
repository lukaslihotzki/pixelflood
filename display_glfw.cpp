#include "display_glfw.hpp"
#include "version.h"

#ifdef USE_GLEW
#include <GL/glew.h>
#endif

#include <stdexcept>
#include <GLFW/glfw3.h>
#include <sys/mman.h>

static const float vertices[][4] = {{-1.f,-1.f,+0.f,+1.f},
                                    {+1.f,-1.f,+1.f,+1.f},
                                    {-1.f,+1.f,+0.f,+0.f},
                                    {+1.f,+1.f,+1.f,+0.f}};

Display::Display(int width, int height, bool fullscreen)
{
	if (glfwInit() != GL_TRUE) {
		throw std::runtime_error("glfwInit failed!");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* vidmode = glfwGetVideoMode(monitor);

	if (fullscreen) {
		width = vidmode->width;
		height = vidmode->height;
	}

	if (width < 0 || height < 0) {
		width = vidmode->width / 3 * 2;
		height = vidmode->height / 3 * 2;
	}

	window = glfwCreateWindow(width, height, RELEASE_NAME, fullscreen ? monitor : nullptr, nullptr);
	if (!window) {
		throw std::runtime_error("glfwCreateWindow failed!");
	}

	glfwSetWindowUserPointer(window, this);

	if (fullscreen) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	glfwMakeContextCurrent(window);

#ifdef USE_GLEW
	glewInit();
#endif

	glfwSetFramebufferSizeCallback(window, [] (GLFWwindow*, int width, int height) {
		glViewport(0, 0, width, height);
	});
	glfwSetKeyCallback(window, [] (GLFWwindow* window, int key, int, int action, int) {
		Display& d = *static_cast<Display*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, 1);
		}
		if (key == GLFW_KEY_DELETE && action == GLFW_PRESS && d.bindCanvas && d.releaseCanvas) {
			d.releaseCanvas();
			d.cleanupTexture();
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			d.createTexture(width, height);
			d.bindCanvas();
		}
	});

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

	createTexture(width, height);
}

Display::~Display()
{
	cleanupTexture();

	glfwDestroyWindow(window);
}

void Display::operator()()
{
	if (bindCanvas && releaseCanvas) {
		bindCanvas();
	}

	do {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texImageBuf);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertices) / sizeof(vertices[0]));
		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));

	if (bindCanvas && releaseCanvas) {
		releaseCanvas();
	}
}

void Display::createTexture(int width, int height)
{
	canvas.width = width;
	canvas.height = height;

	canvas.data = nullptr;

#ifdef USE_GLEW
	if (GL_ARB_buffer_storage) {
		glGenBuffers(1, &buf);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buf);
		GLuint size = width * height * 2 * sizeof(uint32_t);
		GLuint flags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
		glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, nullptr, flags | GL_CLIENT_STORAGE_BIT);
		canvas.data = (uint32_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size, flags);
		madvise(canvas.data, size, MADV_HUGEPAGE);
		texImageBuf = nullptr;
	} else
#endif
		texImageBuf = canvas.data = new uint32_t[width * height * 2];
}

void Display::cleanupTexture()
{
#ifdef USE_GLEW
	if (GL_ARB_buffer_storage) {
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		glDeleteBuffers(1, &buf);
	}
#endif
	if (texImageBuf) {
		delete[] texImageBuf;
	}
}
