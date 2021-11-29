#ifndef NETWORK_EPOLL_HPP
#define NETWORK_EPOLL_HPP

#include "canvas.hpp"
#include <atomic>
#include <thread>
#include <stack>
#include <string>

class NetworkHandler
{
	public:
		NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount = 1);
		~NetworkHandler();
	private:
		void work();
		void work_parse(char* buf, pthread_barrier_t* barrier, std::atomic<bool>* running, int* fd);
		std::stack<std::thread> threads;
		Canvas& canvas;
		int epollfd, evfd, serverfd, fd_max;
		uint64_t* state;
		std::string sizeStr;
};

#endif
