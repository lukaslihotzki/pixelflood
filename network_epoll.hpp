#ifndef NETWORK_EPOLL_HPP
#define NETWORK_EPOLL_HPP

#include "canvas.hpp"
#include <atomic>
#include <thread>

class NetworkHandler
{
	public:
		NetworkHandler(Canvas& canvas, uint16_t port, unsigned threads = 1);
		~NetworkHandler();
	private:
		void work();
		std::thread thread;
		Canvas& canvas;
		int epollfd, evfd, serverfd;
		uint64_t* state;
};

#endif
