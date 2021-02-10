//
// ImageHandler.cc for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "ImageHandler.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "Util.hh"

static Util::StringMap<ImageType> image_type_map =
    {{"", IMAGE_TYPE_NO},
     {"TILED", IMAGE_TYPE_TILED},
     {"SCALED", IMAGE_TYPE_SCALED},
     {"FIXED", IMAGE_TYPE_FIXED}};

ImageHandler::ImageHandler(void)
{
    _images[""] = Util::RefEntry<PImage*>(nullptr);
    clearColorMaps();

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

ImageHandler::~ImageHandler(void)
{
    if (_images.size() != 1) {
        ERR("ImageHandler not empty on destruct, " << _images.size() - 1
              << " entries left");

        while (_images.size()) {
            auto it = _images.begin();
            if (it->second.get()) {
                ERR("delete lost image " << it->first.str());
                delete it->second.get();
            }
            _images.erase(it);
        }
    }

    PImage::loaderClear();
}

/**
 * Gets image from cache and increments the reference or creates a new image.
 */
PImage*
ImageHandler::getImage(const std::string &file)
{
    uint ref;
    return getImage(file, ref, _images);
}

PImage*
ImageHandler::getImage(const std::string &file, uint &ref,
                       Util::StringMap<Util::RefEntry<PImage*>> &images)
{
    if (! file.size()) {
        ref = 0;
        return nullptr;
    }

    std::string real_file(file);
    ImageType image_type = IMAGE_TYPE_TILED;

    // Split image in path # type parts.
    auto pos = file.rfind('#');
    if (std::string::npos != pos) {
        real_file = file.substr(0, pos);
        image_type = image_type_map.get(file.substr(pos + 1));
    }

    // Load the image, try load paths if not an absolute image path
    // already.
    PImage *image = nullptr;
    if (real_file[0] == '/') {
        image = getImageFromPath(real_file, ref, images);
    } else {
        auto it(_search_path.rbegin());
        for (; ! image && it != _search_path.rend(); ++it) {
            image = getImageFromPath(*it + real_file, ref, images);
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
ImageHandler::getImageFromPath(const std::string &file, uint &ref,
                               Util::StringMap<Util::RefEntry<PImage*>> &images)
{
    // Check cache for entry.
    auto entry = images.get(file);
    if (entry.get() != nullptr) {
        ref = entry.incRef();
        return entry.get();
    }

    // Try to load the image, setup cache only if it succeeds.
    PImage *image;
    try {
        image = new PImage(file);
    } catch (LoadException&) {
        image = nullptr;
    }

    // Create new PImage and handler entry for it.
    if (image) {
        images.emplace(std::make_pair(file, Util::RefEntry<PImage*>(image)));
        ref = 1;
    } else {
        ref = 0;
    }

    return image;
}

/**
 * Return image to handler, removes entry if it is the last refernce.
 */
void
ImageHandler::returnImage(PImage *image)
{
    returnImage(image, _images);
}

/**
 * Take ownership ower image.
 */
void
ImageHandler::takeOwnership(PImage *image)
{
    std::string key = Util::to_string<void*>(static_cast<void*>(image));
    _images.emplace(std::make_pair(key, Util::RefEntry<PImage*>(image)));
}

PImage*
ImageHandler::getMappedImage(const std::string &file,
                             const std::string &colormap)
{
    auto it = _images_mapped.find(colormap);
    if (it == _images_mapped.end()) {
        _images_mapped[colormap] = Util::StringMap<Util::RefEntry<PImage*>>();
        _images_mapped[colormap][""] = Util::RefEntry<PImage*>(nullptr);
    }

    uint ref;
    auto image = getImage(file, ref, _images_mapped[colormap]);
    if (ref == 1) {
        // new image, requires color mapping.
        mapColors(image, _color_maps.get(colormap));
    }

    return image;
}

/**
 * Map colors in loaded image.
 */
void
ImageHandler::mapColors(PImage *image, const std::map<int,int> &color_map)
{
    int *p = reinterpret_cast<int*>(image->getData());
    int num_pixels = image->getWidth() * image->getHeight();
    for (; num_pixels; num_pixels--, p++) {
        auto it = color_map.find(*p);
        if (it != color_map.end()) {
            *p = it->second;
        }
    }
}

/**
 * Return color-mapped image to handler, removes entry if it is the
 * last reference.
 */
void
ImageHandler::returnMappedImage(PImage *image, const std::string &colormap)
{
    auto images = _images_mapped.get(colormap);
    returnImage(image, images);
}

void
ImageHandler::returnImage(PImage *image,
                          Util::StringMap<Util::RefEntry<PImage*>> &images)
{
    auto it = images.begin();
    for (; it != images.end(); ++it) {
        if (it->second.get() == image) {
            it->second.decRef();
            if (it->second.getRef() == 0) {
                delete it->second.get();
                images.erase(it);
            }
            return;
        }
    }

    ERR("returned image " << image << " not found in handler");
    delete image;
}
