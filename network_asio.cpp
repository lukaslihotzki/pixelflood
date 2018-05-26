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

connection::connection(my_asio::ip::tcp::socket&& socket, Canvas& canvas, my_asio::const_buffers_1& sizeStrBuf)
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
	socket.async_read_some(my_asio::buffer(buf + o, sizeof(buf) - o), [this] (my_asio::error_code err, size_t size) {
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
						if (x >= canvas.width) {
							goto err;
						}
						EXPECT_CH(' ');
						DEC_NUM(y);
						if (y >= canvas.height) {
							goto err;
						}
						EXPECT_CH(' ');
						HEX_BYTE(r);
						HEX_BYTE(g);
						HEX_BYTE(b);
						if (it != end && *it < 32) {
							OPTIONAL_CH('\r');
							EXPECT_CH('\n');
							canvas.set(x, y, r << 24 | g << 16 | b << 8);
						} else {
							HEX_BYTE(a);
							OPTIONAL_CH('\r');
							EXPECT_CH('\n');
							canvas.blend(x, y, r << 24 | g << 16 | b << 8 | a);
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
						my_asio::async_write(socket, sizeStrBuf, [this] (my_asio::error_code err, size_t) {
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
			destroy();
		}
	});
}

server::server(my_asio::io_service& io_service, my_asio::ip::tcp::endpoint endpoint, Canvas& canvas)
	: acceptor(io_service, endpoint)
	, next_client(io_service)
	, sizeStr([&canvas] () {
		std::ostringstream os;
		os << "SIZE " << canvas.width << ' ' << canvas.height << '\n';
		return os.str();
	} ())
	, sizeStrBuf(my_asio::buffer(sizeStr))
	, canvas(canvas)
{
	accept();
}

void server::accept()
{
	acceptor.async_accept(next_client, [this] (my_asio::error_code err) {
		if (!err) {
			connections.emplace_front(std::move(next_client), canvas, sizeStrBuf);
			auto it = connections.begin();
			it->destroy = [this, it] () { connections.erase(it); };
		}
		accept();
	});
}

NetworkHandler::NetworkHandler(Canvas& canvas, uint16_t port, unsigned threadCount)
	: s(io_service, {my_asio::ip::address_v6::any(), port}, canvas)
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
