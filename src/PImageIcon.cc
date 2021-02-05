//
// PImage.hh for pekwm
// Copyright (C) 2007-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "PImageIcon.hh"
#include "x11.hh"

/**
 * PImageIcon constructor.
 *
 * @param dpy Display to load icon from.
 */
PImageIcon::PImageIcon()
  : PImage()
{
    _type = IMAGE_TYPE_SCALED;
    _use_alpha = true;
}

/**
 * PImage destructor.
 */
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
    if (X11::getProperty(win, X11::getAtom(NET_WM_ICON), XA_CARDINAL,
                         expected, &udata, &actual)) {
        if (actual >= expected) {
            status = setImageFromData(udata, actual);
        }

        X11::free(udata);
    }
    return status;
}

/**
 * Do the actual reading and loading of the icon data in ARGB data.
 */
bool
PImageIcon::setImageFromData(uchar *udata, ulong actual)
{
    // Icon size successfully read, proceed with loading the actual
    // icon data.
    long *from_data = reinterpret_cast<long*>(udata);
    uint width = from_data[0];
    uint height = from_data[1];
    uint pixels = width * height;
    if (actual < (pixels + 2)) {
        return false;
    }

    _width = width;
    _height = height;

    _data = new uchar[pixels * 4];
    convertToARGB(pixels, from_data, _data);
    _pixmap = createPixmap(_data, _width, _height);
    _mask =  createMask(_data, _width, _height);

    return true;
}

void
PImageIcon::convertToARGB(ulong pixels, long *from_data, uchar *to_data)
{
    long *src = from_data;
    uchar *dst = to_data;
    int pixel;
    for (ulong i = 0; i < pixels; i += 1) {
        pixel = *src++; // in case 64bit system drop the unneeded bits
        *dst++ = pixel >> 24 & 0xff;
        *dst++ = pixel >> 16 & 0xff;
        *dst++ = pixel >> 8 & 0xff;
        *dst++ = pixel & 0xff;
    }
}
