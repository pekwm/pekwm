//
// PImageNativeLoaderJpeg.hh for pekwm
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_LOADER_JPEG_HH_
#define _PIMAGE_NATIVE_LOADER_JPEG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_IMAGE_JPEG

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
                        bool &alpha, bool &use_alpha);
};

#endif // HAVE_IMAGE_JPEG

#endif // _PIMAGE_NATIVE_LOADER_JPEG_HH_
