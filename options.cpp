#include "options.hpp"
#include "version.h"
#include <thread>
#include <string>
#include <iostream>

#define REQUIRE(x) if (!(x)) return false;

template <typename T>
static inline bool parsePositiveNum(const char*& str, T& x, char endchar = '\0')
{
	x = 0;

	while (*str >= '0' && *str <= '9') {
		x = 10 * x + (*str - '0');
		str++;
	}

	return x && (*str == endchar);
}

static inline bool parseResolution(const char*& str, Options::Resolution& r)
{
	REQUIRE(parsePositiveNum(str, r.x, 'x'));
	str++;
	return parsePositiveNum(str, r.y);
}

void Options::toDefault()
{
	buffer = { .x = -1, .y = -1 };
	threadCount = std::thread::hardware_concurrency();
	port = 1234;
	quiet = false;
	fullscreen = false;
}

bool Options::parse(const char* const* argv)
{
	toDefault();

	if (!argv) {
		return true;
	}

	argv++;

	while (*argv) {
		const char* arg = *argv;
		REQUIRE(*(arg++) == '-');
		REQUIRE(*arg);

		while (*arg) {
			switch (*(arg++)) {
				case 'b': {
					if (!*arg)
						REQUIRE(arg = *++argv);
					REQUIRE(parseResolution(arg, buffer));
				} break;
				case 't': {
					if (!*arg)
						REQUIRE(arg = *++argv);
					REQUIRE(parsePositiveNum(arg, threadCount));
				} break;
				case 'p': {
					if (!*arg)
						REQUIRE(arg = *++argv);
					REQUIRE(parsePositiveNum(arg, port));
				} break;
				case 'q': {
					quiet = true;
				} break;
				case 'f': {
					fullscreen = true;
				} break;
				default:
					return false;
			}
		}
		argv++;
	}

	return true;
}

void Options::printUsage(const char* const* argv)
{
	std::cerr << RELEASE_NAME << std::endl;

	std::cerr << "Usage: "
			  << (argv[0] ? argv[0] : "pixelflood")
			  << " [ -b BUFFER_RESOLUTION ] [ -t THREAD_COUNT ] [ -p PORT ] [ -q ] [ -f ]"
			  << std::endl;

	std::cerr << "Resolutions are formatted like 1280x720" << std::endl;
}
