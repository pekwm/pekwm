//
// ImageHandler.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "Handler.hh"
#include "PImage.hh"
#include "ParseUtil.hh"

#include <string>
#include <vector>

class PImage;

/**
 * ImageHandler, a caching and image type transparent image handler.
 */
class ImageHandler {
public:
    ImageHandler(void);
    ~ImageHandler(void);

    /** Add path entry to the search path. */
    void path_push_back(const std::string &path) {
        _search_path.push_back(path);
    }
    /** Remove newest entry in the search path. */
    void path_pop_back(void) { _search_path.pop_back(); }
    /** Remove all entries from the search path. */
    void path_clear(void) { _search_path.clear(); }

    PImage *getImage(const std::string &file);
    void returnImage(PImage *image);

    void freeUnref(void);

private:
    PImage *getImageFromPath(const std::string &file);

     /** List of directories to search. */
    std::vector<std::string> _search_path;
     /** List of loaded images. */
    std::vector<HandlerEntry<PImage*> > _images;

    bool _free_on_return; /**< If true, images are deleted when returned. */

     /**< Type name to type enum map. */
    std::map<ParseUtil::Entry, ImageType> _image_type_map;
};

namespace pekwm
{
    ImageHandler* imageHandler();
}
