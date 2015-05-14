//
// StatusWindow.cc for pekwm
// Copyright (C) 2004-2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PWinObj.hh"
#include "PDecor.hh"
#include "StatusWindow.hh"
#include "x11.hh"
#include "PTexture.hh"
#include "ScreenResources.hh"
#include "Theme.hh"
#include "Workspaces.hh"

#include <algorithm>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

StatusWindow *StatusWindow::_instance = 0;

//! @brief StatusWindow constructor
StatusWindow::StatusWindow(Theme *theme)
    : PDecor(theme, "STATUSWINDOW"),
      _bg(None)
{
    if (_instance) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "StatusWindow(" << this << ")::StatusWindow()" << endl
             << " *** _instance already set: " << _instance << endl;
#endif // DEBUG
    }
    _instance = this;


    // PWinObj attributes
    _type = PWinObj::WO_STATUS;
    setLayer(LAYER_NONE); // hack, goes over LAYER_MENU
    _hidden = true; // don't care about it when changing worskpace etc

    XSetWindowAttributes attr;
    attr.event_mask = None;

    _status_wo = new PWinObj;
    _status_wo->setWindow(XCreateWindow(X11::getDpy(), _window,
                                        0, 0, 1, 1, 0,
                                        CopyFromParent, CopyFromParent, CopyFromParent,
                                        CWEventMask, &attr));

    addChild(_status_wo);
    activateChild(_status_wo);
    _status_wo->mapWindow();

    setTitlebar(STATE_UNSET);
    setFocused(true); // always draw as focused

    Workspaces::insert(this);
}

//! @brief StatusWindow destructor
StatusWindow::~StatusWindow(void)
{
    Workspaces::remove(this);

    // remove ourself from the decor manually, no need to reparent and stuff
    _children.erase(std::remove(_children.begin(), _children.end(), _status_wo), _children.end());

    // free resources
    XDestroyWindow(X11::getDpy(), _status_wo->getWindow());
    delete _status_wo;

    unloadTheme();

    _instance = 0;
}

void
StatusWindow::draw(const std::wstring &text, bool center, const Geometry *gm)
{
    uint width, height;
    PFont *font = _theme->getStatusData()->getFont(); // convenience

    width = font->getWidth(text.c_str()) + 10;
    width = width - (width % 10);
    height = font->getHeight()
             + _theme->getStatusData()->getPad(PAD_UP)
             + _theme->getStatusData()->getPad(PAD_DOWN);

    if ((width != getChildWidth()) || (height != getChildHeight())) {
        resizeChild(width, height);
        render();
    }

    if (center) {
        Geometry head;
        if (! gm) {
            X11::getHeadInfo(X11::getCurrHead(), head);
            gm = &head;
        }

        move(gm->x + (gm->width - _gm.width) / 2, gm->y + (gm->height - _gm.height) / 2);
    }

    font->setColor(_theme->getStatusData()->getColor());
    _status_wo->clear();
    font->draw(_status_wo->getWindow(),
               (width - font->getWidth(text.c_str())) / 2,
               _theme->getStatusData()->getPad(PAD_UP),
               text.c_str());
}

//! @brief Renders and sets background
void
StatusWindow::loadTheme(void)
{
    render();
}

//! @brief Frees theme resources
void
StatusWindow::unloadTheme(void)
{
    X11::freePixmap(_bg);
}

//! @brief Renders and sets background
void
StatusWindow::render(void)
{
    X11::freePixmap(_bg);
    _bg = X11::createPixmap(_status_wo->getWidth(), _status_wo->getHeight());
    _theme->getStatusData()->getTexture()->render(_bg, 0, 0, _status_wo->getWidth(), _status_wo->getHeight());

    _status_wo->setBackgroundPixmap(_bg);
    _status_wo->clear();
}
