//
// PImageLoaderJpeg.hh for pekwm
// Copyright (C) 2005-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGELOADERJPEG_HH_
#define _PEKWM_PIMAGELOADERJPEG_HH_

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_JPEG

#include "Types.hh"

#include <cstdio>
#include <string>

/**
 * Jpeg Loader class.
 */
namespace PImageLoaderJpeg
{
	const char *getExt(void);
	uchar* load(const std::string &file, uint &width, uint &height,
		    bool &use_alpha);
}

#endif // PEKWM_HAVE_IMAGE_JPEG

#endif // _PEKWM_PIMAGELOADERJPEG_HH_
