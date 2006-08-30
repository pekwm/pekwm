//
// PImageNativeLoaderJpeg.hh for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_JPEG

#ifndef _PIMAGE_NATIVE_LOADER_JPEG_HH_
#define _PIMAGE_NATIVE_LOADER_JPEG_HH_

#include "pekwm.hh"
#include "PImageNativeLoader.hh"

#include <cstdio>

//! @brief Jpeg Loader class.
class PImageNativeLoaderJpeg : public PImageNativeLoader
{
public:
    PImageNativeLoaderJpeg(void);
    virtual ~PImageNativeLoaderJpeg(void);

    virtual uchar *load(const std::string &file, uint &width, uint &height,
                        bool &alpha);
};

#endif // _PIMAGE_NATIVE_LOADER_JPEG_HH_

#endif // HAVE_IMAGE_JPEG
