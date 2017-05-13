#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include <cstdint>

class Options
{
	public:
		void toDefault();
		bool parse(const char* const* argv);
		void printUsage(const char* const* argv);

		struct Resolution {
				int x, y;
		} buffer;

		unsigned threadCount;
		uint16_t port;
		bool quiet;
		bool fullscreen;
};

#endif
