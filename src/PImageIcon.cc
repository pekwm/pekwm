//
// PImage.hh for pekwm
// Copyright © 2007-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#include "Atoms.hh"
#include "PImageIcon.hh"

using std::cerr;
using std::endl;

//! @brief PImageIcon constructor.
//! @param dpy Display to load icon from.
PImageIcon::PImageIcon(Display *dpy)
  : PImage(dpy)
{
    _type = IMAGE_TYPE_SCALED;
    _has_alpha = true;
    _use_alpha = true;
}

//! @brief PImage destructor.
PImageIcon::~PImageIcon(void)
{
}

/**
 * Load icon from window (if atom is set)
 */
bool
PImageIcon::loadFromWindow(Window win)
{
    bool status = false;
    uchar *udata = 0;
    ulong expected = 2, actual;
    if (AtomUtil::getProperty(win, Atoms::getAtom(NET_WM_ICON), XA_CARDINAL,
                              expected, &udata, &actual)) {
        if (expected == actual) {
            // Icon size successfully read, proceed with loading the
            // actual icon data.
            uint width = udata[0];
            uint height = udata[1];
            expected += width * height;
            status = loadActualFromWindow(win, expected, width, height);
        }

        XFree(udata);
    }

    return status;
}

/**
 * Do the actual reading and loading of the icon data in ARGB data.
 */
bool
PImageIcon::loadActualFromWindow(Window win, ulong expected,
                                 uint width, uint height)
{
    bool status = false;
    uchar *udata = 0;
    ulong actual;

    if (AtomUtil::getProperty(win, Atoms::getAtom(NET_WM_ICON), XA_CARDINAL,
                              expected, &udata, &actual)) {
        if (expected == actual) {
            long *from_data = reinterpret_cast<long*>(udata);

            _width = width;
            _height = height;

            _data = new uchar[_width * _height * 4];
            convertARGBtoRGBA(expected, from_data, _data);

            _pixmap = createPixmap(_data, _width, _height);
            _mask =  createMask(_data, _width, _height);

            status = true;
        }

        XFree(udata);
    }

    return status;
}

/**
 * Convert data, source data is ARGB one pixel per 32bit and
 * destination is RGBA in 4x8bit.
 */
void
PImageIcon::convertARGBtoRGBA(ulong size, long *from_data, uchar *to_data)
{
    uchar *p = to_data;
    int pixel;
    for (ulong i = 2; i < size; i += 1) {
        pixel = from_data[i]; // in case 64bit system drop the unneeded bits
        *p++ = pixel >> 16 & 0xff;
        *p++ = pixel >> 8 & 0xff;
        *p++ = pixel & 0xff;
        *p++ = pixel >> 24 & 0xff;
    }
}
