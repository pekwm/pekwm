//
// Copyright © 2004-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ColorHandler.hh"
#include "PScreen.hh"
#include <cstring>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::list;
using std::string;

ColorHandler* ColorHandler::_instance = 0;

//! @brief ColorHandler constructor
ColorHandler::ColorHandler(Display *dpy)
    : _dpy(dpy),
      _free_on_return(false)
{
#ifdef DEBUG
    if (_instance) {
        cerr << __FILE__ << "@" << __LINE__ << ": " << endl
             << "ColorHandler(" << this << ")::ColorHandler(" << _dpy << ")"
             << endl << " *** _instance allready set" << endl;
    }
#endif // DEBUG
    _instance = this;
    
    // setup default color
    _xc_default.pixel = PScreen::instance()->getBlackPixel();
    _xc_default.red = _xc_default.green = _xc_default.blue = 0;
}

//! @brief ColorHandler destructor
ColorHandler::~ColorHandler(void)
{
    freeColors(true);

    _instance = 0;
}

//! @brief Gets or allocs a color
XColor*
ColorHandler::getColor(const std::string &color)
{
    // check for allready existing entry
    list<ColorHandler::Entry*>::iterator it(_color_list.begin());

    if (strcasecmp(color.c_str(), "EMPTY") == 0) {
        return &_xc_default;
    }

    for (; it != _color_list.end(); ++it) {
        if (*(*it) == color) {
            (*it)->incRef();
            return (*it)->getColor();
        }
    }

    // create new entry
    ColorHandler::Entry *entry = new ColorHandler::Entry(color);
    entry->incRef();

    // X alloc
    XColor dummy;
    if (XAllocNamedColor(_dpy, PScreen::instance()->getColormap(),
                         color.c_str(), entry->getColor(), &dummy) == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "ColorHandler(" << this << ")::getColor(" << color << ")" << endl
             << " *** failed to alloc color: " << color << endl;
#endif // DEBUG
        delete entry;
        entry = 0;
    } else {
        _color_list.push_back(entry);
    }

    return (entry) ? entry->getColor() : &_xc_default;
}

//! @brief Returns a color
void
ColorHandler::returnColor(XColor *xc)
{
    if (&_xc_default == xc) { // no need to return default color
        return;
    }

    list<ColorHandler::Entry*>::iterator it(_color_list.begin());

    for (; it != _color_list.end(); ++it) {
        if ((*it)->getColor() == xc) {
            (*it)->decRef();
            if (_free_on_return && ((*it)->getRef() == 0)) {
                ulong pixels[1] = { (*it)->getColor()->pixel };
                XFreeColors(_dpy, PScreen::instance()->getColormap(), pixels, 1, 0);

                delete *it;
                _color_list.erase(it);
            }
            break;
        }
    }
}

//! @brief Frees color allocated by ColorHandler
//! @param all If false free on only unref colors
void
ColorHandler::freeColors(bool all)
{
    list<ulong> pixel_list;
    list<ColorHandler::Entry*>::iterator it(_color_list.begin());

    for (; it != _color_list.end(); ++it) {
        if (all || ((*it)->getRef() == 0)) {
            pixel_list.push_back((*it)->getColor()->pixel);
            delete *it;
        }
    }

    if (pixel_list.size() > 0) {
        ulong *pixels = new ulong[pixel_list.size()];
        copy(pixel_list.begin(), pixel_list.end(), pixels);
        XFreeColors(_dpy, PScreen::instance()->getColormap(),
                    pixels, pixel_list.size(), 0);
        delete [] pixels;
    }
}
