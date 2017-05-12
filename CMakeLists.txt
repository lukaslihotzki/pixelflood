cmake_minimum_required(VERSION 2.6)

project(pixelflood)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if (WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # no console window
endif()

if ("${CMAKE_SYSTEM}" MATCHES "Linux")
set(CAN_USE_EPOLL YES)
else()
set(CAN_USE_EPOLL NO)
endif()

set(USE_EPOLL ${CAN_USE_EPOLL} CACHE BOOL "Use epoll instead of asio (only linux)")
set(USE_EDGE_TRIGGERED_EPOLL NO CACHE BOOL "Use edge triggered epoll")
set(USE_ONESHOT_EPOLL ${CAN_USE_EPOLL} CACHE BOOL "Use oneshot epoll (required for multithreading)")

find_package(Threads)
include(FindFreetype)

find_package(glfw3 3.2 REQUIRED)
find_package(OpenGL REQUIRED)

set(my_SOURCES main.cpp options.cpp options.hpp text.cpp text.hpp canvas.hpp display_glfw.hpp display_glfw.cpp)
set(my_LIBRARIES ${OPENGL_gl_LIBRARY} glfw ${CMAKE_THREAD_LIBS_INIT} ${FREETYPE_LIBRARIES})

if (USE_EPOLL)
    set(my_SOURCES ${my_SOURCES} network_epoll.hpp network_epoll.cpp)
    add_definitions(-DUSE_NETWORK_EPOLL)
    if (USE_EDGE_TRIGGERED_EPOLL)
        add_definitions(-DUSE_EDGE_TRIGGERED_EPOLL)
    endif()
    if (USE_ONESHOT_EPOLL)
        add_definitions(-DUSE_ONESHOT_EPOLL)
    endif()
else()
    set(my_SOURCES ${my_SOURCES} network_asio.hpp network_asio.cpp)
    find_package(Boost REQUIRED COMPONENTS system) # asio
    include_directories(${Boost_INCLUDE_DIRS})
    set(my_LIBRARIES ${my_LIBRARIES} ${Boost_LIBRARIES})

    if (WIN32)
        set(my_LIBRARIES ${my_LIBRARIES} ws2_32 mswsock) # asio
    endif()
endif()


# Run simple cmake converter to put font files into the data segment
# like https://github.com/wjakob/nanogui/blob/f9c3b7a/CMakeLists.txt

# Glob up resource files
file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf")

# Concatenate resource files into a comma separated string
string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=resources.cpp
  -DOUTPUT_H=resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/bin2c.cmake")

# Run bin2c on resource files
add_custom_command(
  OUTPUT resources.cpp resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

set(my_SOURCES ${my_SOURCES} resources.cpp)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})


include_directories(${FREETYPE_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIR} ${GLFW_INCLUDE_DIR})
add_executable(pixelflood ${my_SOURCES})
target_link_libraries(pixelflood ${my_LIBRARIES})
