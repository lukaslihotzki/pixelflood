#include "canvas.hpp"
#include "options.hpp"
#ifdef USE_NETWORK_EPOLL
#include "network_epoll.hpp"
#else
#include "network_asio.hpp"
#endif
#include "display_glfw.hpp"
#include "text.hpp"
#include <iostream>
#include <sstream>

int main(int, char** argv) try
{
	Options opt;
	if (!opt.parse(argv)) {
		opt.printUsage(argv);
		return 1;
	}

	Canvas canvas(opt.buffer.x, opt.buffer.y);

	std::ostringstream os;
	os << "ip:\n"
	   << exec("(ip addr | grep -Po '(?<=inet )[0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*' | grep -v '^127\\.'; ip addr | grep -Po '(?<=inet6 )2[0-9a-f:]*') | sed 's/^/  /'")
	   << "port:\n"
	   << "  tcp " << opt.port << "\n"
	   << "payload:\n"
	   << "  PX $x $y $color\\n";
	writeText(canvas, os.str());

	NetworkHandler networkHandler(canvas, opt.port, opt.threadCount);

	displayCanvas(canvas);
}
catch (const std::exception& e) {
	std::cerr << e.what() << '\n';
	return 1;
}
