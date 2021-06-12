//
// PImageLoaderPng.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGELOADERPNG_HH_
#define _PEKWM_PIMAGELOADERPNG_HH_

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_PNG

#include "pekwm.hh"

#include <cstdio>

/**
 * PNG Loader class.
 */
namespace PImageLoaderPng
{
	const char *getExt(void);

	uchar* load(const std::string &file, size_t &width, size_t &height,
		    bool &use_alpha);
	bool save(const std::string &file,
		  uchar *data, size_t width, size_t height);
}

#endif // PEKWM_HAVE_IMAGE_PNG

#endif // _PEKWM_PIMAGELOADERPNG_HH_
