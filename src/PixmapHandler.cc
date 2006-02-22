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
#include <list>

using std::list;

// PixmapHandler::Entry

//! @brief Constructor for Entry class
PixmapHandler::Entry::Entry(uint width, uint height, uint depth, Pixmap pixmap) :
_width(width), _height(height), _depth(depth), _pixmap(pixmap)
{
}

//! @brief Destructor for Entry class
PixmapHandler::Entry::~Entry(void)
{
	XFreePixmap(PScreen::instance()->getDpy(), _pixmap);
}

// PixmapHandler

//! @brief Constructor for PixmapHandler class
PixmapHandler::PixmapHandler(uint cache_size, uint threshold) :
_cache_size(cache_size), _size_threshold(threshold)
{
}

//! @brief Destructor for PixmapHandler class
PixmapHandler::~PixmapHandler(void)
{
#ifdef DEBUG
	if (_used_list.size() > 0) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Used pixmap list size > 0, size: " << _used_list.size() << endl;
	}
#endif // DEBUG

	list<Entry*>::iterator it;

	for (it = _free_list.begin(); it != _free_list.end(); ++it) {
		delete *it;
	}
	for (it = _used_list.begin(); it != _used_list.end(); ++it) {
		delete *it;
	}
}

//! @brief Sets the cache size and trims the cache if needed
void
PixmapHandler::setCacheSize(uint cache)
{
	_cache_size = cache;

	while (_free_list.size() > _cache_size) {
		delete _free_list.front();
		_free_list.pop_front();
	}
}

//! @brief Sets size threshold
void
PixmapHandler::setThreshold(uint threshold)
{
	_size_threshold = threshold;
}

//! @brief Searches the Pixmap cache for a sutiable Pixmap or creates a new
//! @param width Width of the pixmap
//! @param height Height of the pixmap
//! @param depth Depth of the pixmap
//! @param exact Wheter or not to match the exact size ignoring threshold
Pixmap
PixmapHandler::getPixmap(uint width, uint height, uint depth, bool exact)
{
	Pixmap pixmap = None;

	// search allready created pixmaps
	list<Entry*>::iterator it(_free_list.begin());
	for (; it != _free_list.end(); ++it) {
		if ((*it)->isMatch(width, height, depth, exact ? 0 : _size_threshold)) {
			pixmap = (*it)->getPixmap();

			// move the entry to the used list
			_used_list.push_back(*it);
			_free_list.erase(it);
			break;
		}
	}

	// no entry, create one
	if (pixmap == None) {
		pixmap = XCreatePixmap(PScreen::instance()->getDpy(),
													 PScreen::instance()->getRoot(),
													 width, height, depth);

		_used_list.push_back(new Entry(width, height, depth, pixmap));
	}

	return pixmap;
}

//! @brief Returns the pixmap to the Pixmap cache
void
PixmapHandler::returnPixmap(Pixmap pixmap)
{
	list<Entry*>::iterator it(_used_list.begin());
	for (; it != _used_list.end(); ++it) {
		if ((*it)->getPixmap() == pixmap) {
			Entry *entry = *it;

			// move the pixmap to the back ( front ) of the cache
			_used_list.erase(it);
			_free_list.push_back(entry);
			break;
		}
	}


	// trim the cache
	while (_free_list.size() > _cache_size) {
		delete _free_list.front();
		_free_list.pop_front();
	}
}
