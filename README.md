# pixelflood

A fast, asynchronous implementation of [Pixelflut](https://cccgoe.de/wiki/Pixelflut).

- Uses GLFW and GLEW (optional) to display the buffer
- Uses epoll on linux or boost asio for networking
- Uses FreeType to draw IP address, port, etc., to buffer (optional)

Using epoll, each open TCP connection only takes 8 bytes of user space memory.

epoll can be used edge triggered, but level triggered (default) seems to be fairer across multiple connections.

## How to start (Linux)
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
$ ./pixelflood
```

### GLFW soft dependency
If GLFW is not installed or not found, fbdev display will be used which requires exclusive framebuffer access on `/dev/fb0`. That's more suitable for embedded systems. Probably, you want to make sure GLFW is found (see CMake output).

### Increase open files ulimit
On most systems, one process can't open more than 1024 files (max 1024 connections). To allow more connections, add `/etc/security/limits.d/90-pixelflood.conf` containing:
```
pxuser hard nofile 1000000
```
Replace `pxuser` with the username of the user running pixelflood, and execute
```
$ ulimit -n 1000000
```
in the shell before starting `./pixelflood`. This will allow about 1000000 connections. To see the maximum settable number of open files, read `/proc/sys/fs/file-max`.
