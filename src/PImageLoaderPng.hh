//
// PImageLoaderPng.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#ifdef HAVE_IMAGE_PNG

#include "pekwm.hh"
#include "PImageLoader.hh"

#include <cstdio>

/**
 * PNG Loader class.
 */
class PImageLoaderPng : public PImageLoader
{
public:
    PImageLoaderPng(void);
    virtual ~PImageLoaderPng(void);

    virtual uchar* load(const std::string &file, uint &width, uint &height,
                        bool &use_alpha) override;

private:
    bool checkSignature(std::FILE *fp);

    static const int PNG_SIG_BYTES;
};

#endif // HAVE_IMAGE_PNG
