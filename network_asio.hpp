#ifndef NETWORK_ASIO_HPP
#define NETWORK_ASIO_HPP

#include "canvas.hpp"
#include "my_asio.hpp"
#include <list>
#include <thread>
#include <stack>
#include <functional>

class connection
{
	public:
		connection(my_asio::ip::tcp::socket&& socket, Canvas& canvas, my_asio::const_buffers_1& sizeStrBuf);
		std::function<void()> destroy;
	private:
		void read();
		my_asio::ip::tcp::socket socket;
		Canvas& canvas;
		char buf[32768];
		int o;
		my_asio::const_buffers_1& sizeStrBuf;
		bool pending;
};

class server
{
	public:
		server(my_asio::io_service& io_service, my_asio::ip::tcp::endpoint endpoint, Canvas& canvas);
	private:
		void accept();
		my_asio::ip::tcp::acceptor acceptor;
		my_asio::ip::tcp::socket next_client;
		std::list<connection> connections;
		std::string sizeStr;
		my_asio::const_buffers_1 sizeStrBuf;
		Canvas& canvas;
};

class NetworkHandler
{
	public:
		NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount = 1);
		~NetworkHandler();
	private:
		void work();
		my_asio::io_service io_service;
		server s;
		std::stack<std::thread> threads;
};

#endif
