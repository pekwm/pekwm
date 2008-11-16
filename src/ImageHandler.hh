//
// ImageHandler.hh for pekwm
// Copyright © 2003-2008 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _IMAGE_HANDLER_HH_
#define _IMAGE_HANDLER_HH_

#include "Handler.hh"
#include "ParseUtil.hh"

#include <string>
#include <list>

class PImage;

/**
 * ImageHandler, a caching and image type transparent image handler.
 */
class ImageHandler {
public:
    ImageHandler(void);
    ~ImageHandler(void);

    //! @brief Returns the ImageHandler instance pointer.
    static inline ImageHandler *instance(void) { return _instance; }

    /** Add path entry to the search path. */
    void path_push_back(const std::string &path) { _search_path.push_back(path); }
    /** Remove newest entry in the search path. */
    void path_pop_back(void) { _search_path.pop_back(); }
    /** Remove all entries from the search path. */
    void path_clear(void) { _search_path.clear(); }

    PImage *getImage(const std::string &file);
    void returnImage(PImage *image);

    void freeUnref(void);

private:
    PImage *getImageFromPath(const std::string &file);

private:
    std::list<std::string> _search_path; /**< List of directories to search. */
    std::list<HandlerEntry<PImage*> > _image_list; /**< List of loaded images. */

    bool _free_on_return; /**< If true, images are deleted when returned. */

    std::map<ParseUtil::Entry, ImageType> _image_type_map; /**< Type name to type enum map. */

    static ImageHandler *_instance; /**< Singleton instance pointer. */
};

#endif // _IMAGE_HANDLER_HH_
