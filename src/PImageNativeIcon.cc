//
// PImageNative.hh for pekwm
// Copyright © 2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#include "Atoms.hh"
#include "PImageNativeIcon.hh"

using std::cerr;
using std::endl;

//! @brief PImageNativeIcon constructor.
//! @param dpy Display to load icon from.
PImageNativeIcon::PImageNativeIcon(Display *dpy)
  : PImageNative(dpy)
{
  _type = IMAGE_TYPE_SCALED;
  _has_alpha = true;
}

//! @brief PImageNative destructor.
PImageNativeIcon::~PImageNativeIcon(void)
{
}

//! @brief Load icon from window (if atom is set)
bool
PImageNativeIcon::load(Window win)
{
  Atom icon = EwmhAtoms::instance()->getAtom(NET_WM_ICON);
  uchar *udata = NULL;
  long expected = 2;

  // Start reading icon size
  if (AtomUtil::getProperty(win, icon, XA_CARDINAL, expected, &udata)) {
    CARD32 *data = reinterpret_cast<CARD32*>(udata);
    
    uint width = data[0];
    uint height = data[1];

    XFree(udata);

    // Read the actual icon
    expected += width * height;
    if (AtomUtil::getProperty(win, icon, XA_CARDINAL, expected, &udata)) {
        data = reinterpret_cast<CARD32*>(udata);

        _data = new uchar[width * height * 4];
        _width = width;
        _height = height;

        // Assign data, source data is ARGB one pixel per 32bit and
        // destination is RGBA in 4x8bit.
        uchar *p = _data;
        for (int i = 2; i < expected; i += 1) {
          *p++ = data[i] >> 16 & 0xff;
          *p++ = data[i] >> 8 & 0xff;
          *p++ = data[i] & 0xff;
          *p++ = data[i] >> 24 & 0xff;
        }

        _pixmap = createPixmap(_data, _width, _height);
        _mask =  createMask(_data, _width, _height);

        XFree(udata);
    }
  }
}