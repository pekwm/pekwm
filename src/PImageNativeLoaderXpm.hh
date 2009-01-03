//
// PImageNativeLoaderXpm.hh for pekwm
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_LOADER_XPM_HH_
#define _PIMAGE_NATIVE_LOADER_XPM_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_IMAGE_XPM

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
                        bool &alpha, bool &use_alpha);

private:
    uchar *createXpmToRgbaTable(XpmImage *xpm_image);

private:
    static const uint CHANNELS; //!< Number of channels for Image data.
    static const uint ALPHA_SOLID; //!< Alpha value for no transperency.
    static const uint ALPHA_TRANSPARENT; //!< Alpha value for fully transperency.
    static const char *COLOR_DEFAULT; //!< Default Color if translation fails.
};

#endif // HAVE_IMAGE_XPM

#endif // _PIMAGE_NATIVE_LOADER_XPM_HH_
