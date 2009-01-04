//
// PImageLoaderPng.hh for pekwm
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_LOADER_PNG_HH_
#define _PIMAGE_NATIVE_LOADER_PNG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_IMAGE_PNG

#include "pekwm.hh"
#include "PImageLoader.hh"

#include <cstdio>

//! @brief PNG Loader class.
class PImageLoaderPng : public PImageLoader
{
public:
    PImageLoaderPng(void);
    virtual ~PImageLoaderPng(void);

    virtual uchar *load(const std::string &file, uint &width, uint &height,
                        bool &alpha, bool &use_alpha);

private:
    bool checkSignature(FILE *fp);

    static const int PNG_SIG_BYTES;
};

#endif // HAVE_IMAGE_PNG

#endif // _PIMAGE_NATIVE_LOADER_PNG_HH_
