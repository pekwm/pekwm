//
// PImage.hh for pekwm
// Copyright (C) 2007-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_ICON_HH_
#define _PIMAGE_ICON_HH_

#include "config.h"

#include "PImage.hh"

/**
 * Image loading from X11 windows.
 */
class PImageIcon : public PImage {
public:
    PImageIcon(void);
    virtual ~PImageIcon(void);

    bool loadFromWindow(Window win);

private:
    bool setImageFromData(uchar *data, ulong actual);
    void convertToARGB(ulong size, long *from_data, uchar *to_data);
};

#endif // _PIMAGE_ICON_HH_
