//
// pekwm_dialog.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
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
#include "pekwm_dialog.hh"

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
#include "../tk/ThemeUtil.hh"
#include "../tk/X11Util.hh"

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

PekwmDialog* PekwmDialog::_instance = nullptr;

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

PekwmDialog::PekwmDialog(const std::string &theme_dir,
			 const std::string &theme_variant,
			 const Geometry &gm, int gm_mask, int decorations,
			 bool raise, const PekwmDialogConfig& config)
	: X11App(gm, gm_mask, config.getTitle(),
		 "dialog", "pekwm_dialog", WINDOW_TYPE_DIALOG,
		 nullptr, true),
	  _initial_gm(gm),
	  _theme(_font_handler, _image_handler, _texture_handler,
		 theme_dir, theme_variant),
	  _data(_theme.getDialogData()),
	  _raise(raise)
{
	X11::selectInput(_window, ExposureMask|StructureNotifyMask);

	assert(_instance == nullptr);
	_instance = this;

	if (decorations) {
		MwmHints mwm_hints(MWM_HINTS_DECORATIONS, 0, decorations);
		X11Util::setMwmHints(_window, mwm_hints);
	}

	// TODO: setup size minimum based on image
	initWidgets(config);
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
PekwmDialog::handleEvent(XEvent *ev)
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
	case Expose:
		render();
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
PekwmDialog::resize(uint width, uint height)
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
PekwmDialog::show(void)
{
	render();
	if (_raise) {
		X11::mapRaised(_window);
	} else {
		X11::mapWindow(_window);
	}
}

void
PekwmDialog::render()
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
	_data->getBackground()->render(rend, 0, 0, _gm.width, _gm.height);
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
PekwmDialog::setState(Window window, ButtonState state)
{
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		if ((*it)->setState(window, state)) {
			break;
		}
	}
}

void
PekwmDialog::click(Window window)
{
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		if ((*it)->click(window)) {
			break;
		}
	}
}

void
PekwmDialog::initWidgets(const PekwmDialogConfig& config)
{
	if (! config.getTitle().empty()) {
		TkWidget* widget = 
			new TkText(_data, *this, config.getTitle(), true);
		_widgets.push_back(widget);
	}
	if (config.getImage() != nullptr) {
		TkWidget* widget =
			new TkImage(_data, *this, config.getImage());
		_widgets.push_back(widget);
	}
	if (! config.getMessage().empty()) {
		TkWidget* widget =
			new TkText(_data, *this, config.getMessage(), false);
		_widgets.push_back(widget);
	}
	_widgets.push_back(new TkButtonRow(_data, *this,
					   PekwmDialog::stopDialog,
					   config.getOptions()));
}

void
PekwmDialog::placeWidgets(void)
{
	// height is dependent on the available width, get requested
	// width first.
	uint width = _initial_gm.width;
	std::vector<TkWidget*>::iterator it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		uint width_req = (*it)->widthReq();
		if (width_req && width_req > width) {
			width = width_req;
		}
	}
	if (width == 0) {
		float scale = pekwm::textureHandler()->getScale();
		width = ThemeUtil::scaledPixelValue(scale, WIDTH_DEFAULT);
	}

	uint y = 0;
	it = _widgets.begin();
	for (; it != _widgets.end(); ++it) {
		(*it)->place(0, y, width, 0);
		(*it)->setHeight((*it)->heightReq(width));
		y += (*it)->getHeight();
	}

	PWinObj::resize(std::max(width, _initial_gm.width),
			std::max(y, _initial_gm.height));
}

void
PekwmDialog::themeChanged(const std::string& name, const std::string& variant,
			  float scale)
{
	bool scale_changed = scale != _texture_handler->getScale();
	if (scale_changed) {
		_font_handler->setScale(scale);
		_image_handler->setScale(scale);
		_texture_handler->setScale(scale);
	}
	_theme.load(name, variant, scale_changed /* force */);

	placeWidgets();
	render();
}

void
PekwmDialog::stopDialog(int retcode)
{
	PekwmDialog::instance()->stop(retcode);
}

static void init(Display* dpy, float scale, bool font_default_x11,
		 const std::string &font_charset_override)
{
	_observer_mapping = new ObserverMapping();
	_font_handler = new FontHandler(scale, font_default_x11,
					font_charset_override);
	_image_handler = new ImageHandler(scale);
	_texture_handler = new TextureHandler(scale);
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
	std::cout << " [-dDhitfl] [-o option|-o option...] message"
		  << std::endl;
	std::cout << "  -d --display dpy     Display" << std::endl;
	std::cout << "  -D --decorations str [all|no-border|no-titlebar]"
		  << std::endl;
	std::cout << "  -h --help            Display this information"
		  << std::endl;
	std::cout << "  -i --image           Image under title" << std::endl;
	std::cout << "  -o --option          Option (many allowed)"
		  << std::endl;
	std::cout << "  -t --title           Dialog title" << std::endl;
	std::cout << "  -f --log-file        Set log file" << std::endl;
	std::cout << "  -l --log-level       Set log level" << std::endl;
	exit(ret);
}

static int
parse_decorations(const std::string& decorations)
{
	if (decorations.empty() || decorations == "all") {
		return MWM_DECOR_ALL;
	}

	int mask = MWM_DECOR_BORDER | MWM_DECOR_TITLE;
	std::vector<std::string> toks;
	Util::splitString(decorations, toks, ",");
	std::vector<std::string>::iterator it(toks.begin());
	for (; it != toks.end(); ++it) {
		if (*it == "no-border") {
			mask &= ~MWM_DECOR_BORDER;
		} else if (*it == "no-titlebar") {
			mask &= ~MWM_DECOR_TITLE;
		} else {
			std::cerr << "unknown decor: " << *it << std::endl;
		}
	}

	return mask;
}

int main(int argc, char* argv[])
{
	pledge_x11_required("");

	const char* display = NULL;
	Geometry gm(0, 0, 0, 0);
	int gm_mask = WIDTH_VALUE | HEIGHT_VALUE;
	bool raise;
	std::string title;
	std::string config_file = Util::getEnv("PEKWM_CONFIG_FILE");
	std::string image_name;
	std::vector<std::string> options;
	int decorations = 0;

	const struct option opts[] = {
		{const_cast<char*>("config"), required_argument, nullptr, 'c'},
		{const_cast<char*>("display"), required_argument, nullptr,
		 'd'},
		{const_cast<char*>("decoration"), required_argument, nullptr,
		 'D'},
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
	while ((ch = getopt_long(argc, argv, "c:d:D:g:hi:o:rt:",
				 opts, nullptr)) != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;
		case 'd':
			display = optarg;
			break;
		case 'D':
			decorations = parse_decorations(optarg);
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

	float scale;
	std::string theme_dir, theme_variant;
	bool font_default_x11;
	std::string font_charset_override;
	{
		CfgParser cfg(CfgParserOpt(""));
		cfg.parse(config_file, CfgParserSource::SOURCE_FILE, true);
		std::string theme_path;
		CfgUtil::getScreenScale(cfg.getEntryRoot(), scale);
		CfgUtil::getThemeDir(cfg.getEntryRoot(),
				     theme_dir, theme_variant, theme_path);
		CfgUtil::getFontSettings(cfg.getEntryRoot(),
					 font_default_x11,
					 font_charset_override);
	}

	X11::init(dpy, true);
	init(dpy, scale, font_default_x11, font_charset_override);

	if (gm.width == 0) {
		gm.width = ThemeUtil::scaledPixelValue(scale, WIDTH_DEFAULT);
	}
	if (gm.height == 0) {
		gm.height = ThemeUtil::scaledPixelValue(scale, HEIGHT_DEFAULT);
	}

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
		PekwmDialogConfig config(title, image, message, options);
		PekwmDialog dialog(theme_dir, theme_variant,
				   gm, gm_mask, decorations, raise, config);
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
