//
// Globals.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
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

static bool s_is_startup = true;

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
    void init(AppCtrl* app_ctrl, Display* dpy, const std::string& config_file)
    {
        _config = new Config();
        _config->load(config_file);
        _config->loadMouseConfig(_config->getMouseConfigFile());

        X11::init(dpy, _config->isHonourRandr());

        _hint_wo = new HintWO(X11::getRoot());
        // Create root PWinObj
        _root_wo = new RootWO(X11::getRoot(), _hint_wo);
        PWinObj::setRootPWinObj(_root_wo);

        _auto_properties = new AutoProperties();
        _auto_properties->load();
        _key_grabber = new KeyGrabber();
        _key_grabber->load(_config->getKeyFile());
        _key_grabber->grabKeys(X11::getRoot());

        _font_handler = new FontHandler();
        _image_handler = new ImageHandler();
        _texture_handler = new TextureHandler();
        _theme = new Theme(_font_handler, _image_handler, _texture_handler);

        _harbour = new Harbour(_config, _auto_properties, _root_wo);
        _status_window = new StatusWindow(_theme);

        _action_handler = new ActionHandler(app_ctrl);
    }

    void cleanup()
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
    }

    ActionHandler* actionHandler()
    {
        return _action_handler;
    }

    AutoProperties* autoProperties()
    {
        return _auto_properties;
    }

    Config* config()
    {
        return _config;
    }

    FontHandler* fontHandler()
    {
        return _font_handler;
    }

    Harbour* harbour()
    {
        return _harbour;
    }

    HintWO* hintWo()
    {
        return _hint_wo;
    }

    RootWO* rootWo()
    {
        return _root_wo;
    }

    ImageHandler* imageHandler()
    {
        return _image_handler;
    }

    KeyGrabber* keyGrabber()
    {
        return _key_grabber;
    }

    StatusWindow* statusWindow()
    {
        return _status_window;
    }

    TextureHandler* textureHandler()
    {
        return _texture_handler;
    }

    Theme* theme()
    {
        return _theme;
    }

    bool isStartup()
    {
        return s_is_startup;
    }

    void setIsStartup(bool is_startup)
    {
        s_is_startup = is_startup;
    }
}
