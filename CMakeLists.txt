cmake_minimum_required(VERSION 2.6)

project(pixelflood)

include(CMakeDependentOption)

if (ANDROID_NATIVE_API_LEVEL)

    include(AndroidNdkModules)
    android_ndk_import_module_native_app_glue()
    find_library(log-lib log)

    list(APPEND my_SOURCES android.cpp)
    list(APPEND my_LIBRARIES ${log-lib} android EGL GLESv1_CM native_app_glue freetype)

    include_directories(android/app/src/freetype-2.7.1/include)
    add_subdirectory(android/app/src/freetype-2.7.1)

    set(FREETYPE_FOUND ON)
    set(NETWORK EPOLL)
    set(GLFW_FOUND OFF)

else()

    include(FindFreetype)
    find_package(Threads REQUIRED)
    find_package(OpenGL)
    find_package(glfw3 3.2)
    find_package(GLEW)
    find_package(Boost COMPONENTS system)

    list(APPEND my_SOURCES main.cpp options.cpp options.hpp)
    list(APPEND my_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

    if (glfw3_FOUND)
        list(APPEND DISPLAY_OPTIONS GLFW)
    endif()

    if ("${CMAKE_SYSTEM}" MATCHES "Linux")
        list(APPEND DISPLAY_OPTIONS FBDEV)
        list(APPEND NETWORK_OPTIONS EPOLL)
    endif()

    if (Boost_FOUND)
        list(APPEND NETWORK_OPTIONS BOOST_ASIO)
    endif()

    list(GET DISPLAY_OPTIONS 0 DEFAULT_DISPLAY)
    list(GET NETWORK_OPTIONS 0 DEFAULT_NETWORK)

    set(DISPLAY ${DEFAULT_DISPLAY} CACHE STRING "Library to display buffer")
    set_property(CACHE DISPLAY PROPERTY STRINGS ${DISPLAY_OPTIONS})

    set(NETWORK ${DEFAULT_NETWORK} CACHE STRING "API to run server")
    set_property(CACHE NETWORK PROPERTY STRINGS ${NETWORK_OPTIONS})

endif()

cmake_dependent_option(USE_FREETYPE "Use FreeType to draw text" ON "FREETYPE_FOUND" OFF)

if (NETWORK MATCHES "BOOST_ASIO")
    #add_definitions(-DUSE_BOOST_ASIO)
    list(APPEND my_INCLUDES ${Boost_INCLUDE_DIRS})
    list(APPEND my_LIBRARIES ${Boost_LIBRARIES})
    list(APPEND my_SOURCES network_asio.cpp network_asio.hpp)
endif()

if (NETWORK MATCHES "EPOLL")
    set(USE_EPOLL ON)
    add_definitions(-DUSE_NETWORK_EPOLL)
    list(APPEND my_SOURCES network_epoll.cpp network_epoll.hpp)
endif()

cmake_dependent_option(USE_EDGE_TRIGGERED_EPOLL "Use edge triggered epoll" OFF "USE_EPOLL" OFF)
if (USE_EDGE_TRIGGERED_EPOLL)
    add_definitions(-DUSE_EDGE_TRIGGERED_EPOLL)
endif()

cmake_dependent_option(USE_ONESHOT_EPOLL "Use oneshot epoll (required for multithreading)" ON "USE_EPOLL" OFF)
if (USE_ONESHOT_EPOLL)
    add_definitions(-DUSE_ONESHOT_EPOLL)
endif()

if (DISPLAY MATCHES "GLFW")
    set(USE_GLFW ON)
    add_definitions(-DUSE_GLFW)
    list(APPEND my_INCLUDES ${GLFW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
    list(APPEND my_LIBRARIES glfw ${OPENGL_gl_LIBRARY})
    list(APPEND my_SOURCES display_glfw.cpp display_glfw.hpp)
endif()

cmake_dependent_option(USE_GLEW "Use GLEW to load extension for zero copy" ON "USE_GLFW; GLEW_FOUND" OFF)
if (USE_GLEW)
    add_definitions(-DUSE_GLEW)
    list(APPEND my_INCLUDES ${GLEW_INCLUDE_DIR})
    list(APPEND my_LIBRARIES glfw ${GLEW_LIBRARIES})
endif()

if (DISPLAY MATCHES "FBDEV")
    add_definitions(-DUSE_FBDEV)
    list(APPEND my_SOURCES display_fbdev.cpp display_fbdev.hpp)
endif()

if (USE_FREETYPE)
    # like https://github.com/wjakob/nanogui/blob/f9c3b7a/CMakeLists.txt

    file(GLOB resources "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf")
    string (REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources}")
    string (REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

    set(bin2c_cmdline
      -DOUTPUT_C=resources.cpp
      -DOUTPUT_H=resources.h
      "-DINPUT_FILES=${resources_string}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/bin2c.cmake")

    add_custom_command(
      OUTPUT resources.cpp resources.h
      COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
      DEPENDS ${resources}
      COMMENT "Running bin2c"
      PRE_BUILD VERBATIM)

    list(APPEND my_SOURCES resources.cpp)
    list(APPEND my_INCLUDES ${CMAKE_CURRENT_BINARY_DIR})

    add_definitions(-DUSE_FREETYPE)
    list(APPEND my_INCLUDES ${FREETYPE_INCLUDE_DIRS})
    list(APPEND my_LIBRARIES ${FREETYPE_LIBRARIES})
    list(APPEND my_SOURCES text.cpp text.hpp)
endif()

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # no console window
    set(my_LIBRARIES ${my_LIBRARIES} ws2_32 mswsock) # networking
endif()

include_directories(${my_INCLUDES})

if (ANDROID_NATIVE_API_LEVEL)
    add_library(pixelflood SHARED ${my_SOURCES})
else()
    add_executable(pixelflood ${my_SOURCES})
endif()

target_link_libraries(pixelflood ${my_LIBRARIES})
