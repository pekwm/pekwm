//
// PImageNative.hh for pekwm
// Copyright © 2005-2008 Claes  Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PIMAGE_NATIVE_HH_
#define _PIMAGE_NATIVE_HH_

#include <list>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "PImage.hh"
#include "PImageNativeLoader.hh"

//! @brief Image class with pekwm native backend.
class PImageNative : public PImage {
public:
    PImageNative(Display *dpy, const std::string &path="") throw(LoadException&);
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

protected:
    void drawFixed(Drawable dest, int x, int y, uint width, uint height);
    void drawScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawTiled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaFixed(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaTiled(Drawable dest, int x, int y, uint widht, uint height);

    Pixmap createPixmap(uchar *data, uint width, uint height);
    Pixmap createMask(uchar *data, uint width, uint height);

private:
    XImage *createXImage(uchar *data, uint width, uint height);
    uchar *getScaledData(uint width, uint height);

protected:
    uchar *_data; //!< Data describing image.
    bool _has_alpha; //!< Wheter image has alpha channel.
    bool _use_alpha; //!< Wheter image has alpha < 100%

private:
    static std::list<PImageNativeLoader*> _loader_list; //!< List of loaders.
};

#endif // _PIMAGE_NATIVE_HH_
