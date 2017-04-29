#include "canvas.hpp"
#ifdef USE_NETWORK_EPOLL
#include "network_epoll.hpp"
#else
#include "network_asio.hpp"
#endif
#include "display_glfw.hpp"
#include "text.hpp"
#include <iostream>
#include <sstream>

int main() try
{
	Canvas canvas(1280, 720);

	std::ostringstream os;
	os << "ip:\n"
	   << exec("(ip addr | grep -Po '(?<=inet )[0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*' | grep -v '^127\\.'; ip addr | grep -Po '(?<=inet6 )2[0-9a-f:]*') | sed 's/^/  /'")
	   << "port:\n"
	   << "  tcp 1234\n"
	   << "payload:\n"
	   << "  PX $x $y $color\\n";
	writeText(canvas, os.str());

	NetworkThread networkThread(canvas, 1234);

	displayCanvas(canvas);
}
catch (const std::exception& e) {
	std::cerr << e.what() << '\n';
	return 1;
}
