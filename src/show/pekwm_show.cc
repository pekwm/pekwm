//
// pekwm_show.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#define PEKWM_X11_APP_MAIN
#include "pekwm_show.hh"

#include "CfgParser.hh"
#include "Charset.hh"
#include "Debug.hh"
#include "Util.hh"
#include "X11.hh"
#include "../pekwm_env.hh"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>

#include "../tk/CfgUtil.hh"
#include "../tk/PWinObj.hh"
#include "../tk/Render.hh"
#include "../tk/Theme.hh"
#include "../tk/TkImage.hh"

extern "C" {
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

PekwmShow::PekwmShow(Theme::DialogData* data,
		     const Geometry &gm, int gm_mask,
		     bool raise, const std::vector<std::string>& images,
		     const std::string& title, PImage* image)
	: X11App(gm, gm_mask, title, "show", "pekwm_show", WINDOW_TYPE_NORMAL,
		 nullptr, true),
	  _data(data),
	  _raise(raise),
	  _images(images)
{
	assert(_instance == nullptr);
	_instance = this;
	initWidgets(title, image);

	X11::selectInput(_window,
			 ButtonPressMask|ButtonReleaseMask|
			 KeyPressMask|KeyReleaseMask|
			 StructureNotifyMask);

	Geometry new_gm(_gm);
	_layout.layout(_gm, _widgets, new_gm);
	X11App::resize(new_gm.width, new_gm.height);
}

PekwmShow::~PekwmShow()
{
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		delete *it;
	}
	_instance = nullptr;
}


void
PekwmShow::handleEvent(XEvent *ev)
{
	switch (ev->type) {
	case ButtonPress:
		setState(ev->xbutton.window, BUTTON_STATE_PRESSED);
		break;
	case ButtonRelease:
		click(ev->xbutton.window, ev->xbutton.button);
		break;
	case ConfigureNotify:
		resize(ev->xconfigure.width, ev->xconfigure.height);
		break;
	case EnterNotify:
		setState(ev->xbutton.window, BUTTON_STATE_HOVER);
		break;
	case LeaveNotify:
		setState(ev->xbutton.window, BUTTON_STATE_FOCUSED);
		break;
	case MapNotify:
		break;
	case ReparentNotify:
		break;
	case UnmapNotify:
		break;
	default:
		P_DBG("UNKNOWN EVENT " << ev->type);
		break;
	}
}

void
PekwmShow::resize(uint width, uint height)
{
	if (_gm.width == width && _gm.height == height) {
		return;
	}

	_gm.width = width;
	_gm.height = height;

	// FIXME: first pass calculating "important" height req, then
	// assign size left to image

	int y = 0;
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		(*it)->place((*it)->getX(), y, width, height);
		(*it)->setHeight((*it)->heightReq(width));
		y += (*it)->getHeight();
	}

	render();
}

void
PekwmShow::show()
{
	render();
	if (_raise) {
		X11::mapRaised(_window);
	} else {
		X11::mapWindow(_window);
	}
}

void
PekwmShow::render()
{
	Drawable draw;
	if (hasBuffer()) {
		draw = getRenderDrawable();
	} else {
		if (_background.resize(_gm.width, _gm.height)) {
			setBackground(_background.getDrawable());
		}
		draw = _background.getDrawable();
	}

	X11Render rend(draw, None);
	RenderSurface surface(rend, _gm);
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		(*it)->render(rend, surface);
	}

	if (hasBuffer()) {
		swapBuffer();
	} else {
		X11::clearWindow(_window);
	}
}

void
PekwmShow::setState(Window window, ButtonState state)
{
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		if ((*it)->setState(window, state)) {
			break;
		}
	}
}

void
PekwmShow::click(Window window, int button)
{
}

void
PekwmShow::initWidgets(const std::string& title, PImage* image)
{
	_widgets.push_back(new TkImage(_data, *this, image));
}

void
PekwmShow::stopShow(int retcode)
{
	PekwmShow::instance()->stop(retcode);
}

PekwmShow* PekwmShow::_instance = nullptr;

static void
usage(const char* name, int ret)
{
	std::cout << "usage: " << name;
	std::cout << " [-dhitfl] [-o option|-o option...] message"
		  << std::endl;
	std::cout << "  -d --display dpy    Display" << std::endl;
	std::cout << "  -h --help           Display this information"
		  << std::endl;
	std::cout << "  -i --image          Image under title" << std::endl;
	std::cout << "  -o --option         Option (many allowed)"
		  << std::endl;
	std::cout << "  -t --title          Dialog title" << std::endl;
	std::cout << "  -f --log-file        Set log file." << std::endl;
	std::cout << "  -l --log-level       Set log level." << std::endl;
	exit(ret);
}

int
main(int argc, char* argv[])
{
	const char* display = NULL;
	Geometry gm(0, 0, 100, 100); // FIXME:
	int gm_mask = WIDTH_VALUE|HEIGHT_VALUE;
	bool raise = false;
	std::string config_file;

	static struct option opts[] = {
		{const_cast<char*>("config"), required_argument, nullptr, 'c'},
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("raise"), no_argument, nullptr, 'r'},
		{const_cast<char*>("log-level"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("log-file"), required_argument, nullptr,
		 'f'},
		{nullptr, 0, nullptr, 0}
	};

	Charset::init();
	initEnv(false);

	int ch;
	while ((ch = getopt_long(argc, argv, "c:d:g:hi:o:rt:",
				 opts, nullptr)) != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;
		case 'd':
			display = optarg;
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case 'f':
			if (! Debug::setLogFile(optarg)) {
				std::cerr << "Failed to open log file "
					  << optarg << std::endl;
			}
			break;
		case 'l':
			Debug::setLevel(Debug::getLevel(optarg));
			break;
		case 'r':
			raise = true;
		default:
			usage(argv[0], 1);
			break;
		}
	}

	if (config_file.empty()) {
		config_file = Util::getConfigDir() + "/config";
	}
	Util::expandFileName(config_file);

	std::vector<std::string> images;
	for (int i = optind; i < argc; i++) {
		images.push_back(argv[i]);
	}
	if (images.empty()) {
		usage(argv[0], 1);
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

	std::string theme_dir, theme_variant;
	bool font_default_x11;
	std::string font_charset_override;
	{
		CfgParser cfg(CfgParserOpt(""));
		cfg.parse(config_file, CfgParserSource::SOURCE_FILE, true);
		std::string theme_path;
		CfgUtil::getThemeDir(cfg.getEntryRoot(),
				     theme_dir, theme_variant, theme_path);
		CfgUtil::getFontSettings(cfg.getEntryRoot(),
					 font_default_x11,
					 font_charset_override);
	}

	X11::init(dpy, true);
	initX11App(dpy, font_default_x11, font_charset_override);

	int ret;

	_image_handler->path_push_back("./");
	PImage *image = nullptr;
	image = _image_handler->getImage(images[0]);
	if (image) {
		image->setType(IMAGE_TYPE_FIXED);
		Theme theme(_font_handler, _image_handler,
			    _texture_handler, theme_dir,
			    theme_variant);
		PekwmShow show(theme.getDialogData(), gm, gm_mask,
			       raise, images, images[0], image);
		show.show();
		ret = show.main(60);
		_image_handler->returnImage(image);
	} else {
		std::cerr << "failed to load " << images[0] << std::endl;
		ret = 1;
	}

	cleanupX11App();
	X11::destruct();
	Charset::destruct();

	return ret;
}
