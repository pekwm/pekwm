//
// PImageLoaderXpm.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#ifdef HAVE_IMAGE_XPM

#include "pekwm.hh"

extern "C" {
#include <stdint.h>
#include <X11/xpm.h>
}

/**
 * Xpm Loader class.
 */
namespace PImageLoaderXpm
{
    const char *getExt(void);
    uchar* load(const std::string &file, uint &width, uint &height,
                        bool &use_alpha);
}

#endif // HAVE_IMAGE_XPM
