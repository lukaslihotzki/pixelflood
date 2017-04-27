#ifndef NETWORK_EPOLL_HPP
#define NETWORK_EPOLL_HPP

#include "canvas.hpp"
#include <atomic>
#include <thread>
#include <vector>

class NetworkThread
{
	public:
		NetworkThread(Canvas& canvas, uint16_t port);
		~NetworkThread();
	private:
		void work();
		std::thread thread;
		Canvas& canvas;
		int epollfd, evfd, serverfd;
		uint64_t* state;
};

#endif
