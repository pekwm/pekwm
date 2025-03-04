//
// PImageLoaderPng.hh for pekwm
// Copyright (C) 2005-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGELOADERPNG_HH_
#define _PEKWM_PIMAGELOADERPNG_HH_

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_PNG

#include "Types.hh"

#include <string>

/**
 * PNG Loader class.
 */
namespace PImageLoaderPng
{
	const char *getExt(void);

	uchar* load(const std::string &file, uint &width, uint &height,
		    bool &use_alpha);
	bool save(const std::string &file,
		  uchar *data, uint width, uint height);
}

#endif // PEKWM_HAVE_IMAGE_PNG

#endif // _PEKWM_PIMAGELOADERPNG_HH_
