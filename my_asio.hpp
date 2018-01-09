#ifndef MY_ASIO_HPP
#define MY_ASIO_HPP

#ifdef USE_BOOST_ASIO
#include <boost/asio.hpp>
namespace my_asio {
	using namespace boost::asio;
	typedef boost::system::error_code error_code;
}
#else
#define ASIO_STANDALONE
#include <asio.hpp>
namespace my_asio = asio;
#endif

#endif
