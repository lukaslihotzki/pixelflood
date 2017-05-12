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
			delete[] data;
		}

		inline void set(int x, int y, uint32_t rgbx)
		{
			data[width * y + x] = rgbx;
		}

		inline void blend(int x, int y, uint32_t rgba)
		{
			uint8_t alpha = rgba;

			if (alpha != 255) {
				uint32_t back = data[width * y + x];
				rgba = alphaMultiply(rgba, alpha) + alphaMultiply(back, 255 - alpha);
			}

			set(x, y, rgba);
		}

		const unsigned width;
		const unsigned height;
		uint32_t* const data;

	private:
		inline uint32_t alphaMultiply(uint32_t rgbx, uint8_t alpha)
		{
			uint32_t rb = (((rgbx >> 8) & 0x00FF00FF) * (alpha + 1)) & 0xFF00FF00;
			uint32_t g = (((rgbx >> 8) & 0xFF00) * (alpha + 1)) & 0x00FF0000;
			return rb | g;
		}
};

#endif
