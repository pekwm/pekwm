//
// PImageLoaderJpeg.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_LOADER_JPEG_HH_
#define _PIMAGE_LOADER_JPEG_HH_

#include "config.h"

#ifdef HAVE_IMAGE_JPEG

#include "pekwm.hh"
#include "PImageLoader.hh"

#include <cstdio>

/**
 * Jpeg Loader class.
 */
class PImageLoaderJpeg : public PImageLoader
{
public:
    PImageLoaderJpeg(void);
    virtual ~PImageLoaderJpeg(void);

    virtual uchar* load(const std::string &file, uint &width, uint &height,
                        bool &use_alpha) override;
};

#endif // HAVE_IMAGE_JPEG

#endif // _PIMAGE_LOADER_JPEG_HH_
