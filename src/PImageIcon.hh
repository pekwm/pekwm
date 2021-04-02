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
    PImageIcon(PImage *image);
    virtual ~PImageIcon(void);

    void setOnWindow(Window win);

    static PImageIcon *newFromWindow(Window win);
    static void setOnWindow(Window win, size_t width, size_t height, uchar *data);

private:
    PImageIcon(void);

private:
    bool setImageFromData(uchar *data, ulong actual);

    static Cardinal* newCardinals(uint width, uint height, uchar *data);
    static void fromCardinals(ulong size, Cardinal *from_data, uchar *to_data);
    static void toCardinals(ulong size, uchar *from_data, Cardinal *to_data);

private:
    Cardinal *_cardinals;
};
