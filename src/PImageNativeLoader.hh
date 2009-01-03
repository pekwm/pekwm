//
// PImageNativeLoader.hh for pekwm
// Copyright © 2005-2008 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _PIMAGE_NATIVE_LOADER_HH_
#define _PIMAGE_NATIVE_LOADER_HH_

#include <string>

//! @brief Image loader base class.
class PImageNativeLoader {
public:
    //! @brief PImageNativeLoader constructor.
    PImageNativeLoader(const char *ext) : _ext(ext) { }
    //! @brief PImageNativeLoader destructor.
    virtual ~PImageNativeLoader(void) { }

    //! @brief Return file extension matching loader.
    inline const char *getExt(void) const { return _ext; }

    //! @brief Loads file into data. (empty method, interface)
    virtual uchar *load(const std::string &file, uint &width, uint &height, bool &alpha, bool &use_alpha) {
        return 0;
    }

private:
    const char *_ext;
};

#endif // _PIMAGE_NATIVE_LOADER_HH_

