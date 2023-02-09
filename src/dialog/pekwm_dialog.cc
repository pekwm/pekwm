//
// pekwm_dialog.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

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
#include "../tk/FontHandler.hh"
#include "../tk/ImageHandler.hh"
#include "../tk/PPixmapSurface.hh"
#include "../tk/PWinObj.hh"
#include "../tk/Render.hh"
#include "../tk/TextureHandler.hh"
#include "../tk/Theme.hh"
#include "../tk/X11App.hh"

#include "../tk/TkWidget.hh"
#include "../tk/TkButtonRow.hh"
#include "../tk/TkImage.hh"
#include "../tk/TkText.hh"

extern "C" {
#include <X11/Xutil.h>
#include <assert.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

static const uint WIDTH_DEFAULT = 250;
static const uint HEIGHT_DEFAULT = 50;

static std::string _config_script_path;
static ObserverMapping *_observer_mapping = nullptr;
static FontHandler* _font_handler = nullptr;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}

	ObserverMapping* observerMapping()
	{
		return _observer_mapping;
	}

	FontHandler* fontHandler()
	{
		return _font_handler;
	}

	ImageHandler* imageHandler()
	{
		return _image_handler;
	}

	TextureHandler* textureHandler()
	{
		return _texture_handler;
	}
}

/**
 * Dialog
 *
 * --------------------------------------------
 * | TITLE                                    |
 * --------------------------------------------
 * | Image if any is displayed here           |
 * |                                          |
 * |                                          |
 * |                                          |
 * --------------------------------------------
 * | Message text goes here                   |
 * |                                          |
 * --------------------------------------------
 * |           [Option1] [Option2]            |
 * --------------------------------------------
 *
 */
class PekwmDialog : public X11App {
public:
	PekwmDialog(Theme::DialogData* data,
		    const Geometry &gm,
		    bool raise, const std::string& title, PImage* image,
		    const std::string& message,
		    const std::vector<std::string>& options);
	virtual ~PekwmDialog(void);

	static PekwmDialog *instance(void) { return _instance; }

	virtual void handleEvent(XEvent *ev)
	{
		switch (ev->type) {
		case ButtonPress:
			setState(ev->xbutton.window, BUTTON_STATE_PRESSED);
			break;
		case ButtonRelease:
			click(ev->xbutton.window);
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

	virtual void resize(uint width, uint height)
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

	void show(void)
	{
		render();
		if (_raise) {
			X11::mapRaised(_window);
		} else {
			X11::mapWindow(_window);
		}
	}

	void render(void)
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
		_data->getBackground()->render(rend,
					       0, 0, _gm.width, _gm.height);
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

	void setState(Window window, ButtonState state)
	{
		std::vector<TkWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			if ((*it)->setState(window, state)) {
				break;
			}
		}
	}

	void click(Window window)
	{
		std::vector<TkWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			if ((*it)->click(window)) {
				break;
			}
		}
	}

protected:
	void initWidgets(const std::string& title, PImage* image,
			 const std::string& message,
			 const std::vector<std::string>& options);

	void placeWidgets(void)
	{
		// height is dependent on the available width, get requested
		// width first.
		uint width = _gm.width;
		std::vector<TkWidget*>::iterator it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			uint width_req = (*it)->widthReq();
			if (width_req && width_req > width) {
				width = width_req;
			}
		}
		if (width == 0) {
			width = WIDTH_DEFAULT;
		}

		uint y = 0;
		it = _widgets.begin();
		for (; it != _widgets.end(); ++it) {
			(*it)->place(0, y, width, 0);
			(*it)->setHeight((*it)->heightReq(width));
			y += (*it)->getHeight();
		}

		PWinObj::resize(std::max(width, _gm.width),
				std::max(y, _gm.height));
	}

private:
	static void stopDialog(int retcode);

private:
	Theme::DialogData* _data;
	bool _raise;

	PPixmapSurface _background;
	std::vector<TkWidget*> _widgets;

	static PekwmDialog *_instance;
};

PekwmDialog::PekwmDialog(Theme::DialogData* data,
			 const Geometry &gm,
			 bool raise, const std::string& title, PImage* image,
			 const std::string& message,
			 const std::vector<std::string>& options)
	: X11App(gm, title.empty() ? "pekwm_dialog" : title,
		 "dialog", "pekwm_dialog", WINDOW_TYPE_NORMAL,
		 nullptr, true),
	  _data(data),
	  _raise(raise)
{
	assert(_instance == nullptr);
	_instance = this;
	// TODO: setup size minimum based on image
	initWidgets(title, image, message, options);
	placeWidgets();
}

PekwmDialog::~PekwmDialog(void)
{
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		delete *it;
	}
	_instance = nullptr;
}

void
PekwmDialog::initWidgets(const std::string& title, PImage* image,
			 const std::string& message,
			 const std::vector<std::string>& options)
{
	if (title.size()) {
		_widgets.push_back(new TkText(_data, *this, title, true));
	}
	if (image) {
		_widgets.push_back(new TkImage(_data, *this, image));
	}
	if (message.size()) {
		_widgets.push_back(new TkText(_data, *this, message, false));
	}
	_widgets.push_back(new TkButtonRow(_data, *this,
					   PekwmDialog::stopDialog, options));
}

void
PekwmDialog::stopDialog(int retcode)
{
	PekwmDialog::instance()->stop(retcode);
}

PekwmDialog* PekwmDialog::_instance = nullptr;

static void init(Display* dpy,
		 bool font_default_x11,
		 const std::string &font_charset_override)
{
	_observer_mapping = new ObserverMapping();
	_font_handler =
		new FontHandler(font_default_x11, font_charset_override);
	_image_handler = new ImageHandler();
	_texture_handler = new TextureHandler();
}

static void cleanup()
{
	delete _texture_handler;
	delete _image_handler;
	delete _font_handler;
	delete _observer_mapping;
}

static void usage(const char* name, int ret)
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

int main(int argc, char* argv[])
{
	const char* display = NULL;
	Geometry gm(0, 0, WIDTH_DEFAULT, HEIGHT_DEFAULT);
	int gm_mask = WIDTH_VALUE | HEIGHT_VALUE;
	bool raise;
	std::string title;
	std::string config_file = Util::getEnv("PEKWM_CONFIG_FILE");
	std::string image_name;
	std::vector<std::string> options;

	static struct option opts[] = {
		{const_cast<char*>("config"), required_argument, nullptr, 'c'},
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("geometry"), required_argument, nullptr,
		 'g'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("image"), required_argument, nullptr, 'i'},
		{const_cast<char*>("option"), required_argument, nullptr, 'o'},
		{const_cast<char*>("raise"), no_argument, nullptr, 'r'},
		{const_cast<char*>("title"), required_argument, nullptr, 't'},
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
		case 'g':
			gm_mask |= X11::parseGeometry(optarg, gm);
			break;
		case 'h':
			usage(argv[0], 0);
			break;
		case 'i':
			image_name = optarg;
			Util::expandFileName(image_name);
			break;
		case 'o':
			options.push_back(optarg);
			break;
		case 'r':
			raise = true;
			break;
		case 't':
			title = optarg;
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
		default:
			usage(argv[0], 1);
			break;
		}
	}

	if (config_file.empty()) {
		config_file = Util::getConfigDir() + "/config";
	}
	Util::expandFileName(config_file);

	if (options.empty()) {
		options.push_back("Ok");
	}

	std::string message;
	for (int i = optind; i < argc; i++) {
		if (i > optind) {
			message += ' ';
		}
		message += argv[i];
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
	init(dpy, font_default_x11, font_charset_override);

	_image_handler->path_push_back("./");
	PImage *image = nullptr;
	if (image_name.size()) {
		image = _image_handler->getImage(image_name);
		if (image) {
			image->setType(IMAGE_TYPE_SCALED);
		}
	}

	int ret;
	{
		// run in separate scope to get Theme destructed before cleanup
		Theme theme(_font_handler, _image_handler, _texture_handler,
			    theme_dir, theme_variant);
		PekwmDialog dialog(theme.getDialogData(),
				   gm, raise, title, image, message, options);
		dialog.show();
		ret = dialog.main(60);
	}
	if (image) {
		_image_handler->returnImage(image);
	}

	cleanup();
	X11::destruct();
	Charset::destruct();

	return ret;
}
