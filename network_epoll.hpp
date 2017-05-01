#ifndef NETWORK_EPOLL_HPP
#define NETWORK_EPOLL_HPP

#include "canvas.hpp"
#include <atomic>
#include <thread>
#include <stack>

class NetworkHandler
{
	public:
		NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount = 1);
		~NetworkHandler();
	private:
		void work();
		std::stack<std::thread> threads;
		Canvas& canvas;
		int epollfd, evfd, serverfd;
		uint64_t* state;
		std::string sizeStr;
};

#endif
