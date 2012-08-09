//
// Copyright © 2004-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ColorHandler.hh"
#include "x11.hh"
#include <cstring>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::vector;
using std::string;

ColorHandler* ColorHandler::_instance = 0;

//! @brief ColorHandler constructor
ColorHandler::ColorHandler(void)
    : _free_on_return(false)
{
#ifdef DEBUG
    if (_instance) {
        cerr << __FILE__ << "@" << __LINE__ << ": " << endl
             << "ColorHandler(" << this << ")::ColorHandler(void)"
             << endl << " *** _instance already set" << endl;
    }
#endif // DEBUG
    _instance = this;
    
    // setup default color
    _xc_default.pixel = X11::getBlackPixel();
    _xc_default.red = _xc_default.green = _xc_default.blue = 0;
}

//! @brief ColorHandler destructor
ColorHandler::~ColorHandler(void)
{
    if (_colours.size() > 0) {
        ulong *pixels = new ulong[_colours.size()];
        for (uint i=0; i < _colours.size(); ++i) {
            pixels[i] = _colours[i]->getColor()->pixel;
            delete _colours[i];
        }
        XFreeColors(X11::getDpy(), X11::getColormap(),
                    pixels, _colours.size(), 0);
        delete [] pixels;
    }


    _instance = 0;
}

//! @brief Gets or allocs a color
XColor*
ColorHandler::getColor(const std::string &color)
{
    // check for already existing entry
    vector<ColorHandler::Entry*>::const_iterator it(_colours.begin());

    if (strcasecmp(color.c_str(), "EMPTY") == 0) {
        return &_xc_default;
    }

    for (; it != _colours.end(); ++it) {
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
    if (XAllocNamedColor(X11::getDpy(), X11::getColormap(),
                         color.c_str(), entry->getColor(), &dummy) == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "ColorHandler(" << this << ")::getColor(" << color << ")" << endl
             << " *** failed to alloc color: " << color << endl;
#endif // DEBUG
        delete entry;
        entry = 0;
    } else {
        _colours.push_back(entry);
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

    vector<ColorHandler::Entry*>::iterator it(_colours.begin());

    for (; it != _colours.end(); ++it) {
        if ((*it)->getColor() == xc) {
            (*it)->decRef();
            if (_free_on_return && ((*it)->getRef() == 0)) {
                ulong pixels[1] = { (*it)->getColor()->pixel };
                XFreeColors(X11::getDpy(), X11::getColormap(), pixels, 1, 0);

                delete *it;
                _colours.erase(it);
            }
            break;
        }
    }
}
