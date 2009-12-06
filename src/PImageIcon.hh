//
// PImage.hh for pekwm
// Copyright © 2007-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_ICON_HH_
#define _PIMAGE_NATIVE_ICON_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PImage.hh"

//! @brief Image class with pekwm native backend.
class PImageIcon : public PImage {
public:
    PImageIcon(Display *dpy);
    virtual ~PImageIcon(void);

    bool loadFromWindow(Window win);

private:
    bool loadActualFromWindow(uchar *data, ulong actual);
    void convertARGBtoRGBA(ulong size, long *from_data, uchar *to_data);
};

#endif // _PIMAGE_NATIVE_ICON_HH_
