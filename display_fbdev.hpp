#ifndef DISPLAY_FBDEV_HPP
#define DISPLAY_FBDEV_HPP

#include "canvas.hpp"

class Display
{
	public:
		Display(int width, int height);
		~Display();
		Canvas canvas;
		void operator()();
		int fbfd;
};

#endif
