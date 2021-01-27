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
#include "PImageLoader.hh"

extern "C" {
#include <stdint.h>
#include <X11/xpm.h>
}

/**
 * Xpm Loader class.
 */
class PImageLoaderXpm : public PImageLoader
{
public:
    PImageLoaderXpm(void);
    virtual ~PImageLoaderXpm(void);

    virtual uchar* load(const std::string &file, uint &width, uint &height,
                        bool &use_alpha) override;

private:
    int32_t *createXpmToArgbTable(XpmImage *xpm_image, bool &use_alpha);
};

#endif // HAVE_IMAGE_XPM
