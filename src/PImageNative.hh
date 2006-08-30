//
// PImageNative.hh for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PIMAGE_NATIVE_HH_
#define _PIMAGE_NATIVE_HH_

#include "pekwm.hh"
#include "PImage.hh"
#include "PImageNativeLoader.hh"

#include <list>

//! @brief Image class with pekwm native backend.
class PImageNative : public PImage {
public:
    PImageNative(Display *dpy);
    virtual ~PImageNative(void);

    //! @brief Add loader to loader list.
    static void loaderAdd(PImageNativeLoader *loader) {
        _loader_list.push_back(loader);
    }
    //! @brief Removes and frees all loaders.
    static void loaderClear(void) {
        while (_loader_list.size()) {
            delete _loader_list.back();
            _loader_list.pop_back();
        }
    }

    virtual bool load(const std::string &file);
    virtual void unload(void);
    virtual void draw(Drawable dest, int x, int y,
                      uint width = 0, uint height = 0);
    virtual Pixmap getPixmap(bool &need_free, uint width = 0, uint height = 0);
    virtual Pixmap getMask(bool &need_free, uint width = 0, uint height = 0);
    virtual void scale(uint width, uint height);

private:
    Pixmap createPixmap(uchar *data, uint width, uint height);
    Pixmap createMask(uchar *data, uint width, uint height);
    XImage *createXImage(uchar *data, uint width, uint height);

    uchar *getScaledData(uint width, uint height);

private:
    uchar *_data; //!< Data describing image.

    bool _has_alpha; //!< Wheter image has alpha channel.

    static std::list<PImageNativeLoader*> _loader_list; //!< List of loaders.
};

#endif // _PIMAGE_NATIVE_HH_
