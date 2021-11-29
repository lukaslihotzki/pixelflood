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
#define CLOSED_STATE 0xf

extern "C" {
	uint64_t parse(uint32_t** output, char** str, uint64_t state, char* cout);
}

static void errno_exit(const char *s)
{
	std::ostringstream os;
	os << s << " error " << errno << ", " << strerror(errno);
	throw std::runtime_error(os.str());
}

NetworkHandler::NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount)
	: canvas(canvas)
        , buf((char*)mmap(nullptr, 1<<21, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0))
	, sizeStr([&canvas] () {
		std::ostringstream os;
		os << "SIZE " << canvas.width << ' ' << canvas.height << '\n';
		return os.str();
	} ())
{
	madvise(buf, 1<<21, MADV_HUGEPAGE);
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
		if ((state[i] & CLOSED_STATE) != CLOSED_STATE) {
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
				int size;
				READ ((size = read(fd, buf, 65535)) > 0) {
					buf[size] = '\0';
					char* c = buf;
					state[fd] = parse(&canvas.data, &c, state[fd], nullptr);
					if (((state[fd] & CLOSED_STATE) == CLOSED_STATE) || c != buf + size) {
						std::cout << state[fd] << std::endl;
						state[fd] = CLOSED_STATE;
						close(fd);
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
