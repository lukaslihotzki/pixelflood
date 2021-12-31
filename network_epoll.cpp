#include "network_epoll.hpp"
#include "version.h"

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
#include <sys/signal.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <sstream>

#ifdef USE_EDGE_TRIGGERED_EPOLL
#define PRE_MODE EPOLLET
#define READ while
#define BREAK_IF_ET break
#else
#define PRE_MODE 0
#define READ if
#define BREAK_IF_ET
#endif

#ifdef USE_ONESHOT_EPOLL
#define MODE PRE_MODE | EPOLLONESHOT
#define MAX_EVENTS 1
#else
#define MODE PRE_MODE
#define MAX_EVENTS 16
#endif

#define INITIAL_STATE 0
#define CLOSED_STATE 1 // invalid because comb = 0 => col = 0

static void errno_exit(const char *s)
{
	std::ostringstream os;
	os << s << " error " << errno << ", " << strerror(errno);
	throw std::runtime_error(os.str());
}

NetworkHandler::NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount)
	: canvas(canvas)
	, sizeStr([&canvas] () {
		std::ostringstream os;
		os << "SIZE " << canvas.width << ' ' << canvas.height << '\n';
		return os.str();
	} ())
{
	signal(SIGPIPE, SIG_IGN);

	if ((fd_max = sysconf(_SC_OPEN_MAX)) < 1024) {
		std::cerr << "OPEN_MAX is very low (" << fd_max << "), assuming 1024." << std::endl;
		fd_max = 1024;
	}

	state = new uint64_t[fd_max];
	std::fill_n(state, fd_max, CLOSED_STATE);

	epollfd = epoll_create1(0);
	if (epollfd == -1)
		errno_exit("epoll_create1");

	evfd = eventfd(0, SOCK_NONBLOCK);
	struct epoll_event evee = { .events = EPOLLIN, .data = { .fd = evfd } };
	epoll_ctl(epollfd, EPOLL_CTL_ADD, evfd, &evee);

	struct sockaddr_in6 destAddr = {};
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
	err = epoll_ctl(epollfd, EPOLL_CTL_ADD, serverfd, &serveree);
	if (err < 0)
		errno_exit("epoll_ctl add server");

#ifdef USE_ONESHOT_EPOLL
	for (unsigned i = 0; i < threadCount; i++) {
#endif
		threads.emplace(&NetworkHandler::work, this);
#ifdef USE_ONESHOT_EPOLL
	}
#endif
}

NetworkHandler::~NetworkHandler()
{
	uint64_t one = 1;
	write(evfd, &one, sizeof one);
	while (!threads.empty()) {
		threads.top().join();
		threads.pop();
	}
	uint64_t devnull;
	read(evfd, &devnull, sizeof devnull);

	close(epollfd);
	close(evfd);
	close(serverfd);
	for (int i = 0; i < fd_max; i++) {
		if (state[i] != CLOSED_STATE) {
			close(i);
		}
	}
	delete[] state;
}

void NetworkHandler::work()
{
	const unsigned xmax = canvas.width, ymax = canvas.height;
	const char* sizeData = sizeStr.data();
	int sizeLen = sizeStr.length();

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
					if (clientfd >= fd_max) {
						throw std::runtime_error("invalid fd");
					}
					state[clientfd] = INITIAL_STATE;
					struct epoll_event ee = { .events = EPOLLIN | MODE, .data = { .fd = clientfd } };
					int err;
					err = epoll_ctl(epollfd, EPOLL_CTL_ADD, clientfd, &ee);
					if (err < 0)
						errno_exit("epoll_ctl add client");
#ifdef USE_ONESHOT_EPOLL
					struct epoll_event serveree = { .events = EPOLLIN | MODE, .data = { .fd = serverfd } };
					err = epoll_ctl(epollfd, EPOLL_CTL_MOD, serverfd, &serveree);
					if (err < 0)
						errno_exit("epoll_ctl rearm server");
#endif
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
					if (!s) {
						s = dc;
						dc = 0;
					} else {
						s += 2;
					}
					switch (s) {
						case0:
						case 0:
							if (*c == 'P') c++;
							else if (*c == 'S') { dc = 0; s = 7; c++; goto case7; }
							else if (*c == 'H') { dc = 0; s = 8; c++; goto case8; }
							else { s = 0; break; }
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
								char lc = *c | 0x20;
								if (*c >= '0' && *c <= '9') {
									col = (col << 4) | (*c - '0');
								} else if (lc >= 'a' && lc <= 'f') {
									col = (col << 4) | (lc - 'a' + 0xA);
								} else {
									break;
								}
								dc++;
								c++;
							}
							if (*c == '\r') {
								c++;
								s = 6;
							}
						case 6:
							if (*c == '\n' && (dc == 6 || dc == 8)) {
								c++;
								if (dc == 6) {
									canvas.set(x, y, col << 8);
								} else {
									canvas.blend(x, y, col);
								}
								s = x = y = col = dc = 0;
								goto case0;
							} else {
								break;
							}
						case7:
						case 7:
							switch (dc) {
								case 0:
									if (*c == 'I') c++; else { dc = 0; break; }
								case 1:
									if (*c == 'Z') c++; else { dc = 1; break; }
								case 2:
									if (*c == 'E') c++; else { dc = 2; break; }
								case 3:
									if (*c != '\n') {
										if (*c == '\r') c++; else { dc = 3; break; }
									}
								case 4:
									if (*c == '\n') {
										int pending;
										int err = ioctl(fd, SIOCOUTQ, &pending);
										if (err || pending) {
											break;
										}
										if (write(fd, sizeData, sizeLen) != sizeLen) {
											break;
										}
										s = x = y = col = dc = 0;
										c++;
										goto case0;
									} else { dc = 4; break; }
							}
						case8:
						case 8:
							switch (dc) {
								case 0:
									if (*c == 'E') c++; else { dc = 0; break; }
								case 1:
									if (*c == 'L') c++; else { dc = 1; break; }
								case 2:
									if (*c == 'P') c++; else { dc = 2; break; }
								case 3:
									if (*c != '\n') {
										if (*c == '\r') c++; else { dc = 3; break; }
									}
								case 4:
									if (*c == '\n') {
										int pending;
										int err = ioctl(fd, SIOCOUTQ, &pending);
										if (err || pending) {
											break;
										}
										if (write(fd, HELP_TEXT, HELP_TEXT_SIZE) != HELP_TEXT_SIZE) {
											break;
										}
										s = x = y = col = dc = 0;
										c++;
										goto case0;
									} else { dc = 4; break; }
							}
					}
					if (s <= 2) {
						dc = s;
						s = 0;
					} else {
						s -= 2;
					}
					if (c == buf + size) {
						comb = s * 9 + dc;
						assert(!(col & ~0xFFFFFFFF));
						assert(!(x & ~0x1FFF));
						assert(!(y & ~0x1FFF));
						assert(!(comb & ~0x3F));
						state[fd] = col | (uint64_t(x) << 32) | (uint64_t(y) << 45) | (uint64_t(comb) << 58);
					} else {
						size = 0;
						BREAK_IF_ET;
					}
				}
				if (size == 0) {
					state[fd] = CLOSED_STATE;
					close(fd);
#ifdef USE_ONESHOT_EPOLL
				} else {
					struct epoll_event ee = { .events = EPOLLIN | MODE, .data = { .fd = fd } };
					int err = epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ee);
					if (err < 0)
						errno_exit("epoll_ctl rearm client");
#endif
				}
			}
		}
	}
}
