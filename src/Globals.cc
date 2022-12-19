//
// Globals.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"

#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Config.hh"
#include "FontHandler.hh"
#include "Harbour.hh"
#include "ImageHandler.hh"
#include "ManagerWindows.hh"
#include "KeyGrabber.hh"
#include "StatusWindow.hh"
#include "TextureHandler.hh"
#include "Theme.hh"

static bool s_is_starting = true;

static ObserverMapping* _observer_mapping = nullptr;

static ActionHandler* _action_handler = nullptr;
static AutoProperties* _auto_properties = nullptr;
static Config* _config = nullptr;
static FontHandler* _font_handler = nullptr;
static Harbour* _harbour = nullptr;
static HintWO* _hint_wo = nullptr;
static ImageHandler* _image_handler = nullptr;
static KeyGrabber* _key_grabber = nullptr;
static RootWO* _root_wo = nullptr;
static StatusWindow* _status_window = nullptr;
static TextureHandler* _texture_handler = nullptr;
static Theme* _theme = nullptr;

namespace pekwm
{
	void initNoDisplay(void)
	{
		_observer_mapping = new ObserverMapping();
	}

	void cleanupNoDisplay(void)
	{
		delete _observer_mapping;
	}

	bool init(AppCtrl* app_ctrl, EventLoop* event_loop,
		  Display* dpy, const std::string& config_file,
		  bool replace, bool synchronous)
	{
		initNoDisplay();

		// configuration parsing require X11 to get atom and
		// resource variables expanded properly
		X11::init(dpy, synchronous, true);

		_config = new Config();
		_config->load(config_file);
		_config->loadMouseConfig(_config->getMouseConfigFile());

		// setup options in X11 from configuration as a second
		// pass
		X11::setHonourRandr(_config->isHonourRandr());

		_hint_wo = new HintWO(X11::getRoot());
		if (! _hint_wo->claimDisplay(replace)) {
			delete _config;
			delete _hint_wo;
			X11::destruct();
			return false;
		}

		// Create root PWinObj
		_root_wo = new RootWO(X11::getRoot(), _hint_wo, _config);
		PWinObj::setRootPWinObj(_root_wo);

		_key_grabber = new KeyGrabber();
		_key_grabber->load(_config->getKeyFile());
		_key_grabber->grabKeys(X11::getRoot());

		_font_handler =
			new FontHandler(_config->isDefaultFontX11(),
					_config->getFontCharsetOverride());
		_image_handler = new ImageHandler();
		_texture_handler = new TextureHandler();
		_theme = new Theme(_font_handler, _image_handler,
				   _texture_handler,
				   _config->getThemeFile(),
				   _config->getThemeVariant());

		_auto_properties = new AutoProperties(_image_handler);
		_auto_properties->load();

		_harbour = new Harbour(_config, _auto_properties, _root_wo);
		_status_window = new StatusWindow(_theme);

		_action_handler = new ActionHandler(app_ctrl, event_loop);

		return true;
	}

	void cleanup(void)
	{
		delete _action_handler;
		delete _harbour;
		delete _status_window;
		delete _theme;
		delete _texture_handler;
		delete _image_handler;
		delete _font_handler;
		delete _key_grabber;
		delete _auto_properties;

		delete _root_wo;
		PWinObj::setRootPWinObj(nullptr);
		delete _hint_wo;

		X11::destruct();

		delete _config;

		cleanupNoDisplay();
	}

	ActionHandler* actionHandler(void)
	{
		return _action_handler;
	}

	AutoProperties* autoProperties(void)
	{
		return _auto_properties;
	}

	Config* config(void)
	{
		return _config;
	}

	FontHandler* fontHandler(void)
	{
		return _font_handler;
	}

	Harbour* harbour(void)
	{
		return _harbour;
	}

	HintWO* hintWo(void)
	{
		return _hint_wo;
	}

	RootWO* rootWo(void)
	{
		return _root_wo;
	}

	ImageHandler* imageHandler(void)
	{
		return _image_handler;
	}

	KeyGrabber* keyGrabber(void)
	{
		return _key_grabber;
	}

	ObserverMapping* observerMapping(void)
	{
		return _observer_mapping;
	}

	StatusWindow* statusWindow(void)
	{
		return _status_window;
	}

	TextureHandler* textureHandler(void)
	{
		return _texture_handler;
	}

	Theme* theme(void)
	{
		return _theme;
	}

	bool isStarting(void)
	{
		return s_is_starting;
	}

	void setStarted(void)
	{
		s_is_starting = false;
	}
}
