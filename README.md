# pixelflood

A fast, asynchronous implementation of [Pixelflut](https://cccgoe.de/wiki/Pixelflut).

- Uses GLFW to display the buffer
- Uses epoll on linux or boost asio on other systems for networking
- Uses FreeType to draw IP address, port, etc., to buffer

Using epoll, each open TCP connection only takes 8 bytes of user space memory.

epoll can be used edge triggered, but level triggered (default) seems to be fairer across multiple connections.

## How to start
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ ./pixelflood
```
