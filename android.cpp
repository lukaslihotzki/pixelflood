#include <jni.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android_native_app_glue.h>
#include <android/window.h>

#include <sys/resource.h>

#include "canvas.hpp"
#include "network_epoll.hpp"

#ifdef USE_FREETYPE
#include "text.hpp"
#include <sstream>
#endif

struct UserData
{
	struct android_app* app;

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLint width;
	EGLint height;

	Canvas* canvas;
	NetworkHandler* networkHandler;
};

static const float vertices[][4] = {{-1.f,-1.f,+0.f,+1.f},
                                    {+1.f,-1.f,+1.f,+1.f},
                                    {-1.f,+1.f,+0.f,+0.f},
                                    {+1.f,+1.f,+1.f,+0.f}};

void init(UserData* userData)
{
	ANativeActivity_setWindowFlags(userData->app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	static const EGLint attribs[] = {
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_NONE
	};

	static const EGLint attribList[] = {
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};

	userData->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(userData->display, 0, 0);

	EGLConfig config;
	EGLint numConfigs;
	eglChooseConfig(userData->display, attribs, &config, 1, &numConfigs);

	EGLint format;
	eglGetConfigAttrib(userData->display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(userData->app->window, 0, 0, format);

	userData->surface = eglCreateWindowSurface(userData->display, config, userData->app->window, nullptr);

	userData->context = eglCreateContext(userData->display, config, nullptr, attribList);

	eglMakeCurrent(userData->display, userData->surface, userData->surface, userData->context);

	eglQuerySurface(userData->display, userData->surface, EGL_WIDTH, &userData->width);
	eglQuerySurface(userData->display, userData->surface, EGL_HEIGHT, &userData->height);

	userData->canvas = new Canvas(userData->width, userData->height, new uint32_t[userData->width * userData->height]);
	std::fill_n(userData->canvas->data, userData->canvas->width * userData->canvas->height, 0);

#ifdef USE_FREETYPE
	writeInfoText(*(userData->canvas), 1234);
#endif

	userData->networkHandler = new NetworkHandler(*(userData->canvas), 1234, std::thread::hardware_concurrency());

	glViewport(0, 0, userData->width, userData->height);

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

void draw_frame(UserData* userData)
{
	if (!userData->display) {
		return;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, userData->canvas->width, userData->canvas->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, userData->canvas->data);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, sizeof(vertices) / sizeof(vertices[0]));
	eglSwapBuffers(userData->display, userData->surface);
}

void terminate(UserData* userData)
{
	if (userData->display != EGL_NO_DISPLAY) {
		eglMakeCurrent(userData->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (userData->context != EGL_NO_CONTEXT) {
			eglDestroyContext(userData->display, userData->context);
			userData->context = EGL_NO_CONTEXT;
		}
		if (userData->surface != EGL_NO_SURFACE) {
			eglDestroySurface(userData->display, userData->surface);
			userData->surface = EGL_NO_SURFACE;
		}
		eglTerminate(userData->display);
		userData->display = EGL_NO_DISPLAY;
	}

	if (userData->canvas) {
		delete userData->networkHandler;
		delete[] userData->canvas->data;
		delete userData->canvas;
		userData->canvas = nullptr;
	}
}

void handle_cmd(struct android_app* app, int32_t cmd)
{
	UserData* userData = (UserData*)app->userData;
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			if (userData->app->window) {
				init(userData);
			}
			break;
		case APP_CMD_TERM_WINDOW:
			terminate(userData);
			break;
		case APP_CMD_LOST_FOCUS:
			draw_frame(userData);
			break;
	}
}

extern "C" void android_main(struct android_app* app)
{
	// Raise open files limit because Android makes it difficult to do for end users.
	struct rlimit rlim;
	if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
		rlim.rlim_cur = rlim.rlim_max;
		setrlimit(RLIMIT_NOFILE, &rlim);
	}

	UserData userData;

	memset(&userData, 0, sizeof(userData));
	app->userData = &userData;
	app->onAppCmd = handle_cmd;
	userData.app = app;

	// Make sure glue isn't stripped.
	app_dummy();

	while (!app->destroyRequested) {
		int events;
		struct android_poll_source* source;
		while (ALooper_pollAll(0, nullptr, &events, (void**)&source) >= 0) {
			if (source) {
				source->process(app, source);
			}
		}

		draw_frame(&userData);
	}
}
