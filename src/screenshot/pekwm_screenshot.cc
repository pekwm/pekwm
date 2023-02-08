//
// pekwm_screenshot.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "Util.hh"
#include "X11.hh"

#include "../tk/ImageHandler.hh"
#include "../tk/PImageLoaderPng.hh"

#include <iostream>
#include <iomanip>
#include <sstream>

extern "C" {
#include <sys/types.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xutil.h>
}

static ImageHandler* _image_handler = nullptr;

namespace pekwm
{
	ImageHandler* imageHandler()
	{
		return _image_handler;
	}
}

static void init(Display* dpy)
{
	_image_handler = new ImageHandler();
}

static void cleanup()
{
	delete _image_handler;
}

static void usage(const char* name, int ret)
{
	std::cout << "usage: " << name << " [-dh] [screenshot.png]"
		  << std::endl;
	std::cout << "  -d --display dpy    Display" << std::endl;
	std::cout << "  -h --help           Display this information"
		  << std::endl;
	exit(ret);
}

static std::string get_screenhot_name(const Geometry& gm)
{
	time_t t = time(nullptr);
	tm tm;
	localtime_r(&t, &tm);

	std::ostringstream name;
	name << "pekwm_screenshot-";
	name << std::put_time(&tm, "%Y%m%dT%H%M%S");
	name << "-" << gm.width << "x" << gm.height;
	name << ".png";
	return name.str();
}

static int take_screenshot(const std::string& output)
{
	Geometry gm = X11::getScreenGeometry();
	XImage *ximage = X11::getImage(X11::getRoot(),
				       gm.x, gm.y, gm.width, gm.height,
				       AllPlanes, ZPixmap);
	if (ximage == nullptr) {
		std::cerr << "Failed to take a screenshot" << std::endl;
		return 1;
	}

	PImage *image = new PImage(ximage);
	X11::destroyImage(ximage);

	bool success =
		PImageLoaderPng::save(output, image->getData(),
				      image->getWidth(), image->getHeight());
	return success ? 0 : 1;
}

int main(int argc, char* argv[])
{
	const char* display = NULL;

	static struct option opts[] = {
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "d:h", opts, nullptr)) != -1) {
		switch (ch) {
		case 'd':
			display = optarg;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		default:
			usage(argv[0], 1);
			break;
		}
	}

	Display *dpy = XOpenDisplay(display);
	if (! dpy) {
		std::string actual_display =
			display ? display : Util::getEnv("DISPLAY");
		std::cerr << "Can not open display!" << std::endl
			  << "Your DISPLAY variable currently is set to: "
			  << actual_display << std::endl;
		return 1;
	}

	X11::init(dpy, true);
	init(dpy);

	std::string output;
	if (optind < argc) {
		output = argv[optind];
	} else {
		output = get_screenhot_name(X11::getScreenGeometry());
	}

	int ret = take_screenshot(output);
	if (ret) {
		std::cerr << "failed to write screenshot to " << output
			  << std::endl;
	} else {
		std::cout << "screenshot written to " << output << std::endl;
	}

	cleanup();
	X11::destruct();

	return ret;
}
