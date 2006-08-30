//
// PImageNativeLoaderXpm.hh for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_XPM

#ifndef _PIMAGE_NATIVE_LOADER_XPM_HH_
#define _PIMAGE_NATIVE_LOADER_XPM_HH_

#include "pekwm.hh"
#include "PImageNativeLoader.hh"

extern "C" {
#include <X11/xpm.h>
}

//! @brief Xpm Loader class.
class PImageNativeLoaderXpm : public PImageNativeLoader
{
public:
    PImageNativeLoaderXpm(void);
    virtual ~PImageNativeLoaderXpm(void);

    virtual uchar *load(const std::string &file, uint &width, uint &height,
                        bool &alpha);

private:
    uchar *createXpmToRgbaTable(XpmImage *xpm_image);

private:
    static const uint CHANNELS; //!< Number of channels for Image data.
    static const uint ALPHA_SOLID; //!< Alpha value for no transperency.
    static const uint ALPHA_TRANSPARENT; //!< Alpha value for fully transperency.
    static const char *COLOR_DEFAULT; //!< Default Color if translation fails.
};

#endif // _PIMAGE_NATIVE_LOADER_XPM_HH_

#endif // HAVE_IMAGE_XPM
