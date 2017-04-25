#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <stdint.h>

class Canvas
{
	public:
		Canvas(int width, int height)
			: width(width)
			, height(height)
			, data(new uint32_t[width * height])
		{
		}

		~Canvas()
		{
			delete data;
		}

		uint32_t* const data;
		const unsigned width;
		const unsigned height;
};

#endif
