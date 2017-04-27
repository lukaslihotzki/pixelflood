#ifndef NETWORK_ASIO_HPP
#define NETWORK_ASIO_HPP

#include "canvas.hpp"
#include <boost/asio.hpp>
#include <list>
#include <thread>

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
		std::list<connection> connections;
		Canvas& canvas;
};

class NetworkThread
{
	public:
		NetworkThread(Canvas& canvas, uint16_t port);
		~NetworkThread();
	private:
		void work();
		boost::asio::io_service io_service;
		server s;
		std::thread thread;
};

#endif
