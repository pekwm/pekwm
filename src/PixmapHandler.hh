//
// PixmapHandler.hh for pekwm
// Copyright (C)  2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PIXMAP_HANDLER_HH_
#define _PIXMAP_HANDLER_HH_

#include "pekwm.hh"

#include <list>

class PixmapHandler {
public:
    class Entry {
    public:
        Entry(uint width, uint height, uint depth, Pixmap pixmap);
        ~Entry(void);

        inline Pixmap getPixmap(void) { return _pixmap; }

        inline bool isMatch(uint width, uint height, uint depth, uint thresh) {
            if ((depth == _depth) &&
                    (_width >= width) && (_width <= (width + thresh)) &&
                    (_height >= height) && (_height <= (height + thresh))) {
                return true;
            }
            return false;
        }

    private:
        uint _width, _height, _depth;
        Pixmap _pixmap;
    };

    PixmapHandler(uint cache_size, uint threshold);
    ~PixmapHandler(void);

    Pixmap getPixmap(uint width, uint height, uint depth, bool exact);
    void returnPixmap(Pixmap pixmap);

    void setCacheSize(uint cache);
    void setThreshold(uint threshold);

private:
    uint _cache_size, _size_threshold;

    std::list<PixmapHandler::Entry*> _free_list, _used_list;
};

#endif // _PIXMAP_HANDLER_HH_
