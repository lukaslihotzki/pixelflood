#include "text.hpp"
#include "version.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <stdexcept>
#include <memory>
#include <regex>
#include <sstream>

#ifdef USE_NETWORK_EPOLL
#include <unistd.h>
#include <arpa/inet.h>
#else
#include "my_asio.hpp"
#endif

#include "resources.h" // autogenerated by cmake (bin2c)

std::string exec(const char* cmd)
{
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}

void writeText(Canvas& canvas, std::string text)
{
	FT_Library library;
	if (FT_Init_FreeType(&library)) {
		throw std::runtime_error("FT_Init_FreeType");
	}

	FT_Face face;
	if (FT_New_Memory_Face(library, dejavusans_ttf, dejavusans_ttf_size, 0, &face)) {
		throw std::runtime_error("FT_New_Memory_Face");
	}

	if (FT_Set_Char_Size(face, 30 * 64, 0, 50, 0)) {
		throw std::runtime_error("FT_Set_Char_Size");
	}

	FT_GlyphSlot slot = face->glyph;

	FT_Vector pen;
	pen.x = 0;
	pen.y = face->size->metrics.height;

	for (char c : text) {
		if (c == '\n') {
			pen.x = 0;
			pen.y += face->size->metrics.height;
			continue;
		}
		int error = FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
		if (error)
			continue;

		for (unsigned i = 0; i < slot->bitmap.rows; i++) {
			for (unsigned j = 0; j < slot->bitmap.width; j++) {
				if ((j + slot->bitmap_left + pen.x / 64) >= canvas.width - 1
					|| (i + pen.y / 64 - slot->bitmap_top) >= canvas.height - 1) {
					continue;
				}
				unsigned char c = slot->bitmap.buffer[slot->bitmap.pitch * i + j];
				canvas.set(j + slot->bitmap_left + pen.x / 64, i + pen.y / 64 - slot->bitmap_top, c << 24 | c << 16 | c << 8);
			}
		}

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

std::vector<std::string> extractCidrIpAddresses(std::string output)
{
	std::vector<std::string> res;
	std::regex pre("((([0-9a-f:]+:[0-9a-f:]+)(([0-9]+\\.){3}[0-9]+)?)|(([0-9]+\\.){3}[0-9]+))/[0-9]+");
	std::sregex_iterator next(output.begin(), output.end(), pre);
	std::sregex_iterator end;
	while (next != end) {
		std::smatch match = *next;
		if (!match[2].str().empty()) {
#ifdef USE_NETWORK_EPOLL
			struct in6_addr addr6;
			struct in6_addr loaddr = IN6ADDR_LOOPBACK_INIT;
			int ret = inet_pton(AF_INET6, match[2].str().c_str(), &addr6);
			if (ret == 1 && memcmp(&addr6, &loaddr, sizeof(in6_addr))) {
#else
			my_asio::error_code err;
			auto addr = my_asio::ip::address_v6::from_string(match[2].str(), err);
			if (!err && !addr.is_loopback()) {
#endif
				res.push_back(match[1].str());
			}
		}
		if (!match[6].str().empty()) {
#ifdef USE_NETWORK_EPOLL
			struct in_addr addr;
			int ret = inet_pton(AF_INET, match[6].str().c_str(), &addr);
			if (ret == 1 && htonl(addr.s_addr) != INADDR_LOOPBACK) {
#else
			my_asio::error_code err;
			auto addr = my_asio::ip::address_v4::from_string(match[6].str(), err);
			if (!err && !addr.is_loopback()) {
#endif
				res.push_back(match[1].str());
			}
		}
		next++;
	}

	return res;
}

std::vector<std::string> getIpAddresses()
{
#ifdef _WIN32
	return {}; // "ip addr" doesn't work on windows and "exec" opens terminal window
#endif
	return extractCidrIpAddresses(exec("ip addr"));
}

std::string getHostname()
{
#ifdef USE_NETWORK_EPOLL
	char buf[1024];
	if (!gethostname(buf, sizeof buf)) {
		return buf;
	}
#else
	my_asio::error_code err;
	std::string hostname = my_asio::ip::host_name(err);
	if (!err) {
		return hostname;
	}
#endif
	return "";
}

std::string getInfoText(Canvas& canvas, uint16_t port)
{
	std::string hostname = getHostname();
	std::vector<std::string> ipAddresses = getIpAddresses();
	std::ostringstream os;
	if (!hostname.empty()) {
		os << "hostname:" "\n"
		      "  " << hostname << "\n";
	}
	if (!ipAddresses.empty()) {
		os << "ip:\n";
	}
	for (std::string ip : ipAddresses) {
		os << "  " << ip << "\n";
	}
	os << "port:\n"
	      "  tcp " << port << "\n"
	      "payload:\n"
	      "  PX $x $y $color\\n" "\n"
	      "  SIZE\\n" "\n"
	      "size:" "\n"
	      "  " << canvas.width << "x" << canvas.height << "\n"
	      "server:" "\n"
	      "  " RELEASE_NAME;
	return os.str();
}

void writeInfoText(Canvas& canvas, uint16_t port)
{
	writeText(canvas, getInfoText(canvas, port));
}
