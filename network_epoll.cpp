#include "network_epoll.hpp"

#include "unistd.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/eventfd.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <fstream>
#include <sstream>

#define MAX_EVENTS 16

#ifdef USE_EDGE_TRIGGERED_EPOLL
#define MODE EPOLLET
#define READ while
#else
#define MODE 0
#define READ if
#endif

static void errno_exit(const char *s)
{
	std::ostringstream os;
	os << s << " error " << errno << ", " << strerror(errno);
	throw std::runtime_error(os.str());
}

NetworkThread::NetworkThread(Canvas& canvas, uint16_t port)
    : canvas(canvas)
{
	int fd_max;
	{
		std::ifstream fd_max_stream("/proc/sys/fs/file-max");
		if (!(fd_max_stream >> fd_max)) {
			throw std::runtime_error("Can not read /proc/sys/fs/file-max");
		}
	}
	state = new uint64_t[fd_max + 1];

	epollfd = epoll_create1(0);
	if (epollfd == -1)
		errno_exit("epoll_create1");

	evfd = eventfd(0, SOCK_NONBLOCK);
	struct epoll_event evee = { .events = EPOLLIN, .data = { .fd = evfd } };
	epoll_ctl(epollfd, EPOLL_CTL_ADD, evfd, &evee);

	struct sockaddr_in6 destAddr = {0};
	destAddr.sin6_family = AF_INET6;
	destAddr.sin6_port = htons(port);
	serverfd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);

	int one = 1;
	setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);

	if (serverfd == -1)
		errno_exit("socket");

	int err;

	err = bind(serverfd, (sockaddr*)&destAddr, sizeof(destAddr));
	if (err < 0)
		errno_exit("bind");

	err = listen(serverfd, 1);
	if (err < 0)
		errno_exit("listen");

	struct epoll_event serveree = { .events = EPOLLIN | MODE, .data = { .fd = serverfd } };
	epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &serveree);

	thread = std::thread(&NetworkThread::work, this);
}

NetworkThread::~NetworkThread()
{
	uint64_t one = 1;
	write(evfd, &one, sizeof one);
	thread.join();
	uint64_t devnull;
	read(evfd, &devnull, sizeof devnull);
	delete[] state;
}

void NetworkThread::work()
{
	const unsigned xmax = canvas.width, ymax = canvas.height;

	for (;;) {
		struct epoll_event event[MAX_EVENTS];
		int eventCnt = epoll_wait(epollfd, event, MAX_EVENTS, -1);
		for (int i = 0; i < eventCnt; i++) {
			int fd = event[i].data.fd;

			if (fd == evfd) {
				return;
			} else if (fd == serverfd) {
				int clientfd;
				READ ((clientfd = accept4(serverfd, nullptr, nullptr, SOCK_NONBLOCK)) >= 0) {
					state[clientfd] = 0;
					struct epoll_event ee = { .events = EPOLLIN | MODE, .data = { .fd = clientfd } };
					epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ee);
				}
			} else {
				char buf[32768];
				int size;
				READ ((size = read(fd, &buf, sizeof buf - 1)) > 0) {
					buf[size] = '\0';
					char* c = buf;
					uint64_t ss = state[fd];
					unsigned col = (ss >> 0) & 0xFFFFFFFF;
					unsigned x = (ss >> 32) & 0x1FFF;
					unsigned y = (ss >> 45) & 0x1FFF;
					unsigned comb = (ss >> 58) & 0x3F;
					unsigned s = comb / 9;
					unsigned dc = comb % 9;
					switch (s) {
						for (;;) {
						    case 0:
								if (*c == 'P') c++; else { s = 0; break; }
							case 1:
								if (*c == 'X') c++; else { s = 1; break; }
							case 2:
								if (*c == ' ') c++; else { s = 2; break; }
							case 3:
								while (*c >= '0' && *c <= '9' && dc < 4) {
									x = 10 * x + (*c - '0');
									if (x >= xmax)
										break;
									dc++;
									c++;
								}
								if (dc && *c == ' ') { dc = 0; c++; } else { s = 3; break; }
							case 4:
								while (*c >= '0' && *c <= '9' && dc < 4) {
									y = 10 * y + (*c - '0');
									if (y >= ymax)
										break;
									dc++;
									c++;
								}
								if (dc && *c == ' ') { dc = 0; c++; } else { s = 4; break; }
							case 5:
								s = 5;
								while (dc < 8) {
									if (*c >= '0' && *c <= '9') {
										col = (col << 4) | *c - '0';
										dc++;
										c++;
									} else if (*c >= 'a' && *c <= 'f') {
										col = (col << 4) | *c - 'a' + 0xA;
										dc++;
										c++;
									} else if (*c >= 'A' && *c <= 'F') {
										col = (col << 4) | *c - 'A' + 0xA;
										dc++;
										c++;
									} else {
										break;
									}
								}
								if (*c == '\r') {
									c++;
									s = 6;
								}
							case 6:
								if (*c == '\n' && (dc == 6 || dc == 8)) {
									c++;
									if (dc == 6) {
										col <<= 8;
									}
									canvas.data[y * canvas.width + x] = col;
									s = x = y = col = dc = 0;
								} else {
									break;
								}
						}
					}
					if (c == buf + size) {
						state[fd] = ((col & 0xFFFFFFFFL) << 0) | ((x & 0x1FFFL) << 32) | ((y & 0x1FFFL) << 45) | (((s * 9 + dc) & 0x3FL) << 58);
					} else {
						close(fd);
					}
				}
				if (size == 0) {
					close(fd);
				}
			}
		}
	}
}
