#ifndef DISPLAY_FBDEV_HPP
#define DISPLAY_FBDEV_HPP

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
		int fbfd;
};

#endif
