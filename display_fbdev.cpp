#include "display_fbdev.hpp"

#define FB_NAME "/dev/fb0"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <stdexcept>
#include <sys/signal.h>

static volatile bool running = true;

Display::Display(int width, int height, bool fullscreen)
{
	fbfd = open(FB_NAME, O_RDWR);

	signal(SIGINT, [] (int) { running = false; });

	if (fbfd < 0) {
		throw std::runtime_error("Unable to open " FB_NAME);
	}

	struct fb_fix_screeninfo fixinfo;
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fixinfo) < 0) {
		close(fbfd);
		throw std::runtime_error("get fixed screen info failed");
	}

	struct fb_var_screeninfo varinfo;
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &varinfo) < 0) {
		close(fbfd);
		throw std::runtime_error("get variable screen info failed");
	}

	void* fb = mmap(NULL, fixinfo.line_length * varinfo.yres, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

	if (!fb) {
		printf("mmap failed:\n");
		close(fbfd);
		abort();
	}

	canvas.width = fixinfo.line_length / 4;
	canvas.height = varinfo.yres;
	canvas.data = (uint32_t*)fb;
}

Display::~Display()
{
	close(fbfd);
}

void Display::operator()()
{
	while (running) {
		pause();
	}
}
