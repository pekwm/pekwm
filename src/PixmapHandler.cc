//
// PixmapHandler.cc for pekwm
// Copyright (C)  2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "PixmapHandler.hh"
#include "PScreen.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
#include <map>

using std::map;

// PixmapHandler::Entry

//! @brief Constructor for Entry class
//! @param width Width of entry.
//! @param height Height of entry.
//! @param depth Depth of entry.
//! @param pixmap Pixmap associated with entry.
PixmapHandler::Entry::Entry(uint width, uint height, uint depth, Pixmap pixmap)
    : _width(width), _height(height), _depth(depth), _pixmap(pixmap)
{
}

//! @brief Destructor for Entry class
PixmapHandler::Entry::~Entry(void)
{
    XFreePixmap(PScreen::instance()->getDpy(), _pixmap);
}

// PixmapHandler

//! @brief Constructor for PixmapHandler class
PixmapHandler::PixmapHandler(uint cache_size)
    : _cache_size(cache_size)
{
}

//! @brief Destructor for PixmapHandler class
PixmapHandler::~PixmapHandler(void)
{
#ifdef DEBUG
    if (_used_pix.size() > 0) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Used pixmap list size > 0, size: "
             << _used_pix.size() << endl;
    }
#endif // DEBUG

    // Free resources
    map<Pixmap, Entry*>::iterator it;
    for (it = _free_pix.begin(); it != _free_pix.end(); ++it) {
        delete it->second;
    }
    for (it = _used_pix.begin(); it != _used_pix.end(); ++it) {
        delete it->second;
    }
}

//! @brief Sets the cache size and trims the cache if needed
void
PixmapHandler::setCacheSize(uint cache)
{
    // Cache size will be trimmed on next return pixmap call.
    _cache_size = cache;
}

//! @brief Searches the Pixmap cache for a sutiable Pixmap or creates a new
//! @param width Width of the pixmap
//! @param height Height of the pixmap
//! @param depth Depth of the pixmap
Pixmap
PixmapHandler::getPixmap(uint width, uint height, uint depth)
{
    Pixmap pixmap = None;

    // search already created pixmaps
    map<Pixmap, Entry*>::iterator it(_free_pix.begin());
    for (; it != _free_pix.end(); ++it) {
        if (it->second->isMatch(width, height, depth)) {
            pixmap = it->first;

            // Move the entry to the used list
            _used_pix[it->first] = it->second;
            _free_pix.erase(it);
            break;
        }
    }

    // No entry, create one
    if (pixmap == None) {
        pixmap = XCreatePixmap(PScreen::instance()->getDpy(),
                               PScreen::instance()->getRoot(),
                               width, height, depth);

        _used_pix[pixmap] = new Entry(width, height, depth, pixmap);
    }

    return pixmap;
}

//! @brief Returns the pixmap to the Pixmap cache
void
PixmapHandler::returnPixmap(Pixmap pixmap)
{
    // Remove from used list
    std::map<Pixmap, Entry*>::iterator it = _used_pix.find(pixmap);
    if (it != _used_pix.end()) {
        _free_pix[it->first] = it->second;
        _used_pix.erase(it);
    }

    // Trim the cache
    while (_free_pix.size() > _cache_size) {
        it = _free_pix.begin();

        delete it->second;
        _free_pix.erase(it);
    }
}
