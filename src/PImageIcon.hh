//
// PImage.hh for pekwm
// Copyright (C) 2007-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

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
