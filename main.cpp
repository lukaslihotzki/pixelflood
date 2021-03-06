#include "canvas.hpp"
#include "options.hpp"

#ifdef USE_NETWORK_EPOLL
#include "network_epoll.hpp"
#else
#include "network_asio.hpp"
#endif

#if defined(USE_FBDEV)
#include "display_fbdev.hpp"
#else
#include "display_glfw.hpp"
#endif

#ifdef USE_FREETYPE
#include "text.hpp"
#endif

#include <iostream>

int main(int, char** argv) try
{
	Options opt;
	if (!opt.parse(argv)) {
		opt.printUsage(argv);
		return 1;
	}

	Display display(opt.buffer.x, opt.buffer.y, opt.fullscreen);

	NetworkHandler* networkHandler;

	display.bindCanvas = [&] () {
		std::fill_n(display.canvas.data, display.canvas.width * display.canvas.height, 0);

		if (!opt.quiet) {
#ifdef USE_FREETYPE
			writeInfoText(display.canvas, opt.port);
#endif
		}

		networkHandler = new NetworkHandler(display.canvas, opt.port, opt.threadCount);
	};

	display.releaseCanvas = [&] () {
		delete networkHandler;
	};

	display();
}
catch (const std::exception& e) {
	std::cerr << e.what() << '\n';
	return 1;
}
