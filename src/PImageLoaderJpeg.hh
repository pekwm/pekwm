//
// PImageLoaderJpeg.hh for pekwm
// Copyright © 2005-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_LOADER_JPEG_HH_
#define _PIMAGE_NATIVE_LOADER_JPEG_HH_

#include "config.h"

#ifdef HAVE_IMAGE_JPEG

#include "pekwm.hh"
#include "PImageLoader.hh"

#include <cstdio>

//! @brief Jpeg Loader class.
class PImageLoaderJpeg : public PImageLoader
{
public:
    PImageLoaderJpeg(void);
    virtual ~PImageLoaderJpeg(void);

    virtual uchar *load(const std::string &file, uint &width, uint &height,
                        bool &alpha, bool &use_alpha);
};

#endif // HAVE_IMAGE_JPEG

#endif // _PIMAGE_NATIVE_LOADER_JPEG_HH_
