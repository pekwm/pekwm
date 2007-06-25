//
// PImageNative.hh for pekwm
// Copyright © 2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifndef _PIMAGE_NATIVE_ICON_HH_
#define _PIMAGE_NATIVE_ICON_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PImageNative.hh"

//! @brief Image class with pekwm native backend.
class PImageNativeIcon : public PImageNative {
public:
    PImageNativeIcon(Display *dpy);
    virtual ~PImageNativeIcon(void);

    bool load(Window win);
};

#endif // _PIMAGE_NATIVE_ICON_HH_
