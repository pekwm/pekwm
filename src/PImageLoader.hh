//
// PImageLoader.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_LOADER_HH_
#define _PIMAGE_LOADER_HH_

#include "config.h"

#include <string>

/**
 * Image loader base class.
 */
class PImageLoader {
public:
    PImageLoader(const char *ext)
        : _ext(ext)
    {
    }
    virtual ~PImageLoader(void)
    {
    }

    /**
     * Return file extension matching loader.
     */
    inline const char *getExt(void) const { return _ext; }

    /**
     * Loads file into data in ARGB format.
     */
    virtual uchar* load(const std::string &file, uint &width, uint &height,
                        bool &use_alpha) = 0;

private:
    const char *_ext;
};

#endif // _PIMAGE_LOADER_HH_
