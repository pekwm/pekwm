//
// PImageNativeLoaderPng.hh for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_PNG

#ifndef _PIMAGE_NATIVE_LOADER_PNG_HH_
#define _PIMAGE_NATIVE_LOADER_PNG_HH_

#include "pekwm.hh"
#include "PImageNativeLoader.hh"

#include <cstdio>

//! @brief PNG Loader class.
class PImageNativeLoaderPng : public PImageNativeLoader
{
public:
	PImageNativeLoaderPng(void);
	virtual ~PImageNativeLoaderPng(void);

	virtual uchar *load(const std::string &file, uint &width, uint &height,
											bool &alpha);

private:
	bool checkSignature(FILE *fp);

	static const int PNG_SIG_BYTES;
};

#endif // _PIMAGE_NATIVE_LOADER_PNG_HH_

#endif // HAVE_IMAGE_PNG
