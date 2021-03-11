//
// WinLayouter.hh for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "X11.hh"

#include <string>
#include <vector>

class Frame;

class WinLayouter {
public:
    WinLayouter() {}
    virtual ~WinLayouter() {}

    void layout(Frame *f, Window parent);

protected:
    // temp. variables that get filled in by layout()
    static int _ptr_x, _ptr_y; // mouse pointer coordinates
    static Geometry _gm; // geometry of the head

private:
    bool placeOnParent(Frame *f, Window parent);

    virtual bool layout_impl(Frame *f)=0;
};

WinLayouter *WinLayouterFactory(std::string name);
