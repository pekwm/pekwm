//
// ImageHandler.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "PImage.hh"
#include "Util.hh"

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

    void takeOwnership(PImage *image);

    PImage *getMappedImage(const std::string &file,
                           const std::string& colormap);
    void returnMappedImage(PImage *image, const std::string& colormap);

    void clearColorMaps(void) {
        _color_maps.clear();
        _color_maps.emplace(std::make_pair("", std::map<int,int>()));
    }
    void addColorMap(const std::string& name, std::map<int,int> color_map) {
        _color_maps[name] = color_map;
    }

private:
    PImage *getImage(const std::string &file, uint &ref,
                     Util::StringMap<Util::RefEntry<PImage*>> &images);
    PImage *getImageFromPath(const std::string &file, uint &ref,
                             Util::StringMap<Util::RefEntry<PImage*>> &images);

    void mapColors(PImage *image, const std::map<int,int> &color_map);

    static void returnImage(PImage *image,
                            Util::StringMap<Util::RefEntry<PImage*>> &images);
private:

    /** List of directories to search. */
    std::vector<std::string> _search_path;
    /** Loaded images. */
    Util::StringMap<Util::RefEntry<PImage*>> _images;
    /** Loaded images with color mapped data. */
    Util::StringMap<Util::StringMap<Util::RefEntry<PImage*>>> _images_mapped;

    Util::StringMap<std::map<int, int>> _color_maps;
};

namespace pekwm
{
    ImageHandler* imageHandler();
}
