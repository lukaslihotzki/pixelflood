#ifndef CANVAS_HPP
#define CANVAS_HPP

#include <stdint.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define SWAP_RGBA
#elif __BYTE_ORDER != __BIG_ENDIAN
#error Unknown endianness
#endif

class Canvas
{
	public:
		Canvas()
		{
		}

		Canvas(int width, int height, uint32_t* data)
			: width(width)
			, height(height)
			, data(data)
		{
		}

		inline void set(int x, int y, uint32_t rgbx)
		{
			data[width * y + x] = convert(rgbx);
		}

		inline void blend(int x, int y, uint32_t rgba)
		{
			uint8_t alpha = rgba;
			uint32_t back = data[width * y + x];
			rgba = alphaMultiply(convert(rgba), alpha) + alphaMultiply(back, 255 - alpha);
			data[width * y + x] = rgba;
		}

		unsigned width;
		unsigned height;
		uint32_t* data;

	private:

		inline uint32_t alphaMultiply(uint32_t rgbx, uint8_t alpha)
		{
#ifndef SWAP_RGBA
			rgbx >>= 8;
#endif
			uint32_t rb = ((rgbx & 0x00FF00FF) * (alpha + 1)) & 0xFF00FF00;
			uint32_t g = ((rgbx & 0xFF00) * (alpha + 1)) & 0x00FF0000;
			rgbx = rb | g;
#ifdef SWAP_RGBA
			rgbx >>= 8;
#endif
			return rgbx;
		}

		inline uint32_t convert(uint32_t color)
		{
#ifdef SWAP_RGBA
			return __builtin_bswap32(color);
#else
			return color;
#endif
		}
};

#endif
