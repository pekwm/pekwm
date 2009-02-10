//
// ImageHandler.cc for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#include "Config.hh"
#include "PImage.hh"
#include "ImageHandler.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "PScreen.hh"
#include "Util.hh"

using std::cerr;
using std::endl;
using std::list;
using std::vector;
using std::string;

ImageHandler *ImageHandler::_instance = 0;

//! @brief ImageHandler constructor
ImageHandler::ImageHandler(void)
    : _free_on_return(false)
{
#ifdef DEBUG
    if (_instance) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "ImageHandler(" << this << ")::ImageHandler() *** _instance already set: "
             << _instance << endl;
    }
#endif // DEBUG
    _instance = this;

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
    if (_image_list.size()) {
      cerr << " *** WARNING: ImageHandler not empty, " << _image_list.size() << " entries left:" << endl;

        while (_image_list.size()) {
            cerr << "              * " << _image_list.back().getName() << endl;
            delete _image_list.back().getData();
            _image_list.pop_back();
        }
    }

    PImage::loaderClear();
}

//! @brief Gets or creates a Image
PImage*
ImageHandler::getImage(const std::string &file)
{
    if (! file.size()) {
        return 0;
    }

    string real_file(file);
    ImageType image_type = IMAGE_TYPE_TILED;

    // Split image in path # type parts.
    vector<string> tok;
    if ((Util::splitString(file, tok, "#", 2, true)) == 2) {
        real_file = tok[0];
        image_type = ParseUtil::getValue<ImageType>(tok[1], _image_type_map);
    }

    // Load the image, try load paths if not an absolute image path
    // already.
    PImage *image = 0;
    if (real_file[0] == '/') {
        image = getImageFromPath(real_file);
    } else {
        list<string>::reverse_iterator it(_search_path.rbegin());
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
    list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
    for (; it != _image_list.end(); ++it) {
        if (*it == file) {
            it->incRef();
            return it->getData();
        }
    }

    // Try to load the image, setup cache only if it succeeds.
    PImage *image;
    try {
        image = new PImage(PScreen::instance()->getDpy(), file);
    } catch (LoadException&) {
        image = 0;
    }

    // Create new PImage and handler entry for it.
    if (image) {
        HandlerEntry<PImage*> entry(file);
        entry.incRef();
        entry.setData(image);
        _image_list.push_back(entry);
    }

    return image;
}

//! @brief Returns a Image
void
ImageHandler::returnImage(PImage *image)
{
    bool found = false;

    list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
    for (; it != _image_list.end(); ++it) {
        if (it->getData() == image) {
            found = true;

            it->decRef();
            if (_free_on_return || ! it->getRef()) {
                delete it->getData();
                _image_list.erase(it);
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
    list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
    while (it != _image_list.end()) {
        if (it->getRef() == 0) {
            delete it->getData();
            it = _image_list.erase(it);
        } else {
            ++it;
        }
    }
}
