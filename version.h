// These 3 lines are read by gradle for android builds using regex.
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 3

// This is just for the C++ compiler.
#define STRINGIFY(x) STRINGIFY_PRE(x)
#define STRINGIFY_PRE(x) #x

#define VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR) "." STRINGIFY(VERSION_PATCH)

#define APP_NAME "pixelflood"
#define RELEASE_NAME APP_NAME " " VERSION

#define SOURCE_URL "https://lihotzki.de/pixelflood"

#define SUPPORTED_COMMANDS(indent) \
	indent "PX x y rrggbb\\n" "\n" \
	indent "PX x y rrggbbaa\\n" "\n" \
	indent "SIZE\\n" "\n" \
	indent "HELP\\n" "\n"

#define HELP_TEXT \
	RELEASE_NAME "\n" SOURCE_URL "\n\n" \
	SUPPORTED_COMMANDS("") "\n"

#define HELP_TEXT_SIZE (sizeof(HELP_TEXT) - 1) // exclude terminating null character
