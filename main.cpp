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

#ifdef USE_FREETYPE
	std::ostringstream os;
	os << "ip:\n"
	   << exec("(ip addr | grep -Po '(?<=inet )[0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*' | grep -v '^127\\.'; ip addr | grep -Po '(?<=inet6 )2[0-9a-f:]*') | sed 's/^/  /'")
	   << "port:\n"
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
