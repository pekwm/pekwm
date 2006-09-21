//
// PixmapHandler.hh for pekwm
// Copyright (C)  2003-2006 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#include "../config.h"

#ifndef _PIXMAP_HANDLER_HH_
#define _PIXMAP_HANDLER_HH_

#include "pekwm.hh"

#include <map>

class PixmapHandler {
public:
    class Entry {
    public:
        Entry(uint width, uint height, uint depth, Pixmap pixmap);
        ~Entry(void);

        inline bool isMatch(uint width, uint height, uint depth) {
            return ((depth == _depth) && (_width == width) && (_height == height));
        }

    private:
        uint _width; //!< Width of entry.
        uint _height; //!< Height of entry.
        uint _depth; //!< Depth of entry.
        Pixmap _pixmap; //!< Pixmap associated with entry.
    };

    PixmapHandler(uint cache_size);
    ~PixmapHandler(void);

    Pixmap getPixmap(uint width, uint height, uint depth);
    void returnPixmap(Pixmap pixmap);

    void setCacheSize(uint cache);

private:
    uint _cache_size;

    std::map<Pixmap, Entry*> _free_pix; //!< Set of free pixmaps.
    std::map<Pixmap, Entry*> _used_pix; //!< Set of used pixmaps.
};

#endif // _PIXMAP_HANDLER_HH_
