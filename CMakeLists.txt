cmake_minimum_required(VERSION 2.6)

project(pixelflood)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
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

set(my_SOURCES main.cpp text.cpp canvas.hpp display_glfw.hpp display_glfw.cpp)
set(my_LIBRARIES glfw GL ${CMAKE_THREAD_LIBS_INIT} ${FREETYPE_LIBRARIES})

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
    set(my_LIBRARIES ${my_LIBRARIES} boost_system)
endif()

include_directories(${FREETYPE_INCLUDE_DIRS})
add_executable(pixelflood ${my_SOURCES})
target_link_libraries(pixelflood ${my_LIBRARIES})
