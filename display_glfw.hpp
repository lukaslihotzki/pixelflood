#ifndef DISPLAY_GLFW_HPP
#define DISPLAY_GLFW_HPP

#include "canvas.hpp"
#include <functional>

class Display
{
	public:
		Display(int width, int height, bool fullscreen);
		~Display();
		Canvas canvas;
		void operator()();
		std::function<void()> bindCanvas;
		std::function<void()> releaseCanvas;
	private:
		void createTexture(int width, int height);
		void cleanupTexture();
		struct GLFWwindow* window;
		uint32_t* texImageBuf;
		uint32_t buf;
};

#endif
