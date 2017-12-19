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

