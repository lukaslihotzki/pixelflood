#include "network_asio.hpp"

#define EXPECT_CH(c) \
	if (it != end && *it == c) { \
	it++; \
	} else { \
	goto err; \
	} \

#define OPTIONAL_CH(c) \
	if (it != end && *it == c) { \
	it++; \
	}

#define DEC_NUM(c) \
	unsigned c = 0; \
	while (it != end && *it >= '0' && *it <= '9') { \
	c = (10 * c + (*it - '0')); \
	it++; \
	}

#define HEX_DIGIT(c) \
	if (it != end) { \
	if (*it >= '0' && *it <= '9') { \
	c |= *it - '0'; \
	} else if (*it >= 'a' && *it <= 'f') { \
	c |= *it - 'a' + 0xA; \
	} else if (*it >= 'A' && *it <= 'F') { \
	c |= *it - 'A' + 0xA; \
	} else { \
	goto err; \
	} \
	it++; \
	} else { \
	goto err; \
	}

#define HEX_BYTE(c) \
	uint8_t c = 0; \
	HEX_DIGIT(c); \
	c <<= 4; \
	HEX_DIGIT(c);

connection::connection(boost::asio::ip::tcp::socket&& socket, Canvas& canvas, boost::asio::const_buffers_1& sizeStrBuf)
    : socket(std::move(socket))
    , canvas(canvas)
    , o(0)
	, sizeStrBuf(sizeStrBuf)
	, pending(false)
{
	read();
}

void connection::read()
{
	socket.async_read_some(boost::asio::buffer(buf + o, sizeof(buf) - o), [this] (boost::system::error_code err, size_t size) {
		if (!err) {
			o += size;
			int ro = 0;
			for (int i = o - 1; i >= std::max(0, o - 64); i--) {
				if (buf[i] == '\n') {
					ro = i + 1;
					break;
				}
			}
			if (ro) {
				char* it = buf;
				char* end = buf + ro;
				while (it != end) {
					if (*it == 'P') {
						it++;
						EXPECT_CH('X');
						EXPECT_CH(' ');
						DEC_NUM(x);
						EXPECT_CH(' ');
						DEC_NUM(y);
						EXPECT_CH(' ');
						HEX_BYTE(r);
						HEX_BYTE(g);
						HEX_BYTE(b);
						OPTIONAL_CH('\r');
						EXPECT_CH('\n');
						if (x < canvas.width && y < canvas.height) {
	#if __BYTE_ORDER == __LITTLE_ENDIAN
							canvas.data[canvas.width * y + x] = r << 24 | g << 16 | b << 8;
	#elif __BYTE_ORDER == __BIG_ENDIAN
							canvas.data[canvas.width * y + x] = r << 0 | g << 8 | b << 16;
	#else
	#error Unknown endianness
	#endif
						}
					} else if (*it == 'S') {
						it++;
						EXPECT_CH('I');
						EXPECT_CH('Z');
						EXPECT_CH('E');
						OPTIONAL_CH('\r');
						EXPECT_CH('\n');
						if (pending) {
							goto err;
						}

						pending = true;
						boost::asio::async_write(socket, sizeStrBuf, [this] (boost::system::error_code err, size_t) {
							if (err) {
								socket.close();
								destroy();
							}
							pending = false;
						});
					} else {
						goto err;
					}
				}
				goto ok;
                err:
				socket.close();
				destroy();
				return;
                ok:
				std::copy(buf + ro, buf + o, buf);
				o -= ro;
				read();
			} else if (o < 64) {
				read();
			} else {
				socket.close();
				destroy();
			}
		} else {
			socket.close();
			destroy();
		}
	});
}

server::server(boost::asio::io_service& io_service, boost::asio::ip::tcp::endpoint endpoint, Canvas& canvas)
    : acceptor(io_service, endpoint)
    , next_client(io_service)
	, sizeStr([&canvas] () {
		std::ostringstream os;
		os << "SIZE " << canvas.width << ' ' << canvas.height << '\n';
		return os.str();
	} ())
	, sizeStrBuf(boost::asio::buffer(sizeStr))
    , canvas(canvas)
{
	accept();
}

void server::accept()
{
	acceptor.async_accept(next_client, [this] (boost::system::error_code err) {
		if (!err) {
			connection* c = new connection(std::move(next_client), canvas, sizeStrBuf);
			c->destroy = [c] () { delete c; };
		}
		accept();
	});
}

NetworkHandler::NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount)
	: s(io_service, {boost::asio::ip::address_v6::any(), port}, canvas)
{
	for (unsigned i = 0; i < threadCount; i++) {
		threads.emplace(&NetworkHandler::work, this);
	}
}

NetworkHandler::~NetworkHandler()
{
	io_service.stop();
	while (!threads.empty()) {
		threads.top().join();
		threads.pop();
	}
}

void NetworkHandler::work()
{
	io_service.run();
}
