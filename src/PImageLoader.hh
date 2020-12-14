//
// PImageLoader.hh for pekwm
// Copyright (C) 2005-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_LOADER_HH_
#define _PIMAGE_NATIVE_LOADER_HH_

#include "config.h"

#include <string>

//! @brief Image loader base class.
class PImageLoader {
public:
    //! @brief PImageLoader constructor.
    PImageLoader(const char *ext) : _ext(ext) { }
    //! @brief PImageLoader destructor.
    virtual ~PImageLoader(void) { }

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

