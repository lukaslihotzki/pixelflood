#include "canvas.hpp"
#include "options.hpp"

#ifdef USE_NETWORK_EPOLL
#include "network_epoll.hpp"
#else
#include "network_asio.hpp"
#endif

#if defined(USE_FBDEV)
#include "display_fbdev.hpp"
#elif defined(USE_DRM)
#include "display_drm.hpp"
#else
#include "display_glfw.hpp"
#endif

#ifdef USE_FREETYPE
#include "text.hpp"
#endif

#include <iostream>
#include <sstream>

int main(int, char** argv) try
{
	Options opt;
	if (!opt.parse(argv)) {
		opt.printUsage(argv);
		return 1;
	}

	Display display(opt.buffer.x, opt.buffer.y);
	std::fill_n(display.canvas.data, display.canvas.width * display.canvas.height, 0);

#ifdef USE_FREETYPE
	std::ostringstream os;
	os << "ip:\n";
	for (std::string ip : getIpAddresses())
		os << "  " << ip << "\n";
	os << "port:\n"
	   << "  tcp " << opt.port << "\n"
	   << "payload:\n"
	   << "  PX $x $y $color\\n";
	writeText(display.canvas, os.str());
#endif

	NetworkHandler networkHandler(display.canvas, opt.port, opt.threadCount);

	display();
}
catch (const std::exception& e) {
	std::cerr << e.what() << '\n';
	return 1;
}
