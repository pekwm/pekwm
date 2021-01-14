//
// PImage.hh for pekwm
// Copyright (C) 2007-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "PImageIcon.hh"
#include "x11.hh"

//! @brief PImageIcon constructor.
//! @param dpy Display to load icon from.
PImageIcon::PImageIcon()
  : PImage()
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
    if (X11::getProperty(win, NET_WM_ICON, XA_CARDINAL,
                         expected, &udata, &actual)) {
        if (actual >= expected) {
            status = setImageFromData(udata, actual);
        }

        XFree(udata);
    }

    return status;
}

/**
 * Do the actual reading and loading of the icon data in ARGB data.
 */
bool
PImageIcon::setImageFromData(uchar *udata, ulong actual)
{
    // Icon size successfully read, proceed with loading the actual icon data.
    long *from_data = reinterpret_cast<long*>(udata);
    uint width = from_data[0];
    uint height = from_data[1];
    if (actual < (width * height + 2)) {
        return false;
    }

    _width = width;
    _height = height;

    _data = new uchar[_width * _height * 4];
    convertARGBtoRGBA(_width * _height, from_data, _data);

    _pixmap = createPixmap(_data, _width, _height);
    _mask =  createMask(_data, _width, _height);

    return true;
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
