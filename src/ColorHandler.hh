//
// ColorHandler.hh for pekwm
// Copyright © 2004-2008 Claes Nasten <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _COLOR_HANDLER_HH_
#define _COLOR_HANDLER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include <list>
#include <string>
#include <cstring>

class ColorHandler {
public:
    class Entry {
    public:
        Entry(const std::string &name) : _name(name), _ref(0) { }
        ~Entry(void) { }

        inline XColor *getColor(void) { return &_xc; }

        inline uint getRef(void) const { return _ref; }
        inline void incRef(void) { _ref++; }
        inline void decRef(void) { if (_ref > 0) {_ref--; }
        }

        inline bool operator==(const std::string &name) {
            return (strcasecmp(_name.c_str(), name.c_str()) == 0);
        }

    private:
        std::string _name;
        XColor _xc;

        uint _ref;
    };

    ColorHandler(Display *dpy);
    ~ColorHandler(void);

    static ColorHandler *instance(void) { return _instance; }

    inline bool isFreeOnReturn(void) const { return _free_on_return; }
    inline void setFreeOnReturn(bool free) { _free_on_return = free; }

    XColor *getColor(const std::string &color);
    void returnColor(XColor *xc);

    void freeColors(bool all);

private:
    Display *_dpy;

    XColor _xc_default; // when allocating fails
    std::list<ColorHandler::Entry*> _color_list;
    bool _free_on_return; // used when returning many colours

    static ColorHandler *_instance;
};

#endif // _COLOR_HANDLER_HH_
