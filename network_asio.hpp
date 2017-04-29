#ifndef NETWORK_ASIO_HPP
#define NETWORK_ASIO_HPP

#include "canvas.hpp"
#include <boost/asio.hpp>
#include <list>
#include <thread>
#include <stack>

class connection
{
	public:
		connection(boost::asio::ip::tcp::socket&& socket, Canvas& canvas);
		std::function<void()> destroy;
	private:
		void read();
		boost::asio::ip::tcp::socket socket;
		Canvas& canvas;
		char buf[32768];
		int o;
};

class server
{
	public:
		server(boost::asio::io_service& io_service, boost::asio::ip::tcp::endpoint endpoint, Canvas& canvas);
	private:
		void accept();
		boost::asio::ip::tcp::acceptor acceptor;
		boost::asio::ip::tcp::socket next_client;
		Canvas& canvas;
};

class NetworkHandler
{
	public:
		NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount = 1);
		~NetworkHandler();
	private:
		void work();
		boost::asio::io_service io_service;
		server s;
		std::stack<std::thread> threads;
};

#endif
