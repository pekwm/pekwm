//
// ImageHandler.cc for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "ImageHandler.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "Util.hh"

//! @brief ImageHandler constructor
ImageHandler::ImageHandler(void)
    : _free_on_return(false)
{
    // setup parsing maps
    _image_type_map[""] = IMAGE_TYPE_NO;
    _image_type_map["TILED"] = IMAGE_TYPE_TILED;
    _image_type_map["SCALED"] = IMAGE_TYPE_SCALED;
    _image_type_map["FIXED"] = IMAGE_TYPE_FIXED;

#ifdef HAVE_IMAGE_JPEG
    PImage::loaderAdd(new PImageLoaderJpeg());
#endif // HAVE_IMAGE_JPEG
#ifdef HAVE_IMAGE_PNG
    PImage::loaderAdd(new PImageLoaderPng());
#endif // HAVE_IMAGE_PNG
#ifdef HAVE_IMAGE_XPM
    PImage::loaderAdd(new PImageLoaderXpm());
#endif // HAVE_IMAGE_XPM
}

//! @brief ImageHandler destructor
ImageHandler::~ImageHandler(void)
{
    if (_images.size()) {
        std::cerr << " *** WARNING: ImageHandler not empty, " << _images.size()
                  << " entries left:" << std::endl;

        while (_images.size()) {
            std::cerr << "              * " << _images.back().getName()
                      << std::endl;
            delete _images.back().getData();
            _images.pop_back();
        }
    }

    PImage::loaderClear();
}

//! @brief Gets or creates a Image
PImage*
ImageHandler::getImage(const std::string &file)
{
    if (! file.size()) {
        return nullptr;
    }

    std::string real_file(file);
    ImageType image_type = IMAGE_TYPE_TILED;

    // Split image in path # type parts.
    auto pos = file.rfind('#');
    if (std::string::npos != pos) {
        real_file = file.substr(0, pos);
        image_type = ParseUtil::getValue<ImageType>(file.substr(pos + 1),
                                                    _image_type_map);
    }

    // Load the image, try load paths if not an absolute image path
    // already.
    PImage *image = nullptr;
    if (real_file[0] == '/') {
        image = getImageFromPath(real_file);
    } else {
        auto it(_search_path.rbegin());
        for (; ! image && it != _search_path.rend(); ++it) {
            image = getImageFromPath(*it + real_file);
        }
    }

    // Image was found, set correct type.
    if (image) {
        image->setType(image_type);
    }

    return image;
}

/**
 * Load image from absolute path, checks cache for hit before loading.
 *
 * @param file Path to image file.
 * @return PImage or 0 if fails.
 */
PImage*
ImageHandler::getImageFromPath(const std::string &file)
{
    // Check cache for entry.
    for (auto it : _images) {
        if (it == file) {
            it.incRef();
            return it.getData();
        }
    }

    // Try to load the image, setup cache only if it succeeds.
    PImage *image;
    try {
        image = new PImage(file);
    } catch (LoadException&) {
        image = 0;
    }

    // Create new PImage and handler entry for it.
    if (image) {
        HandlerEntry<PImage*> entry(file);
        entry.incRef();
        entry.setData(image);
        _images.push_back(entry);
    }

    return image;
}

//! @brief Returns a Image
void
ImageHandler::returnImage(PImage *image)
{
    bool found = false;

    auto it(_images.begin());
    for (; it != _images.end(); ++it) {
        if (it->getData() == image) {
            found = true;

            it->decRef();
            if (_free_on_return || ! it->getRef()) {
                delete it->getData();
                _images.erase(it);
            }
            break;
        }
    }

    if (! found) {
        delete image;
    }
}

//! @brief Frees all images not beeing in use
void
ImageHandler::freeUnref(void)
{
    auto it(_images.begin());
    while (it != _images.end()) {
        if (it->getRef() == 0) {
            delete it->getData();
            it = _images.erase(it);
        } else {
            ++it;
        }
    }
}
