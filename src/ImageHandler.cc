//
// ImageHandler.cc for pekwm
// Copyright (C) 2003-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "ImageHandler.hh"
#include "PImage.hh"
#include "Util.hh"

extern "C" {
#include <assert.h>
}

static Util::StringTo<ImageType> image_type_map[] =
	{{"TILED", IMAGE_TYPE_TILED},
	 {"SCALED", IMAGE_TYPE_SCALED},
	 {"FIXED", IMAGE_TYPE_FIXED},
	 {nullptr, IMAGE_TYPE_NO}};

ImageRefEntry::ImageRefEntry(const std::string& u_name, PImage* data)
	: _u_name(u_name),
	  _data(data),
	  _ref(1)
{
	assert(_data);
}

ImageRefEntry::~ImageRefEntry(void)
{
}

uint
ImageRefEntry::incRef(void)
{
	_ref++;
	return _ref;
}

uint
ImageRefEntry::decRef(void)
{
	if (_ref > 0) {
		_ref--;
	}
	return _ref;
}

ImageHandler::ImageHandler(void)
{
	clearColorMaps();
}

ImageHandler::~ImageHandler(void)
{
	if (! _images.empty()) {
		P_ERR("ImageHandler not empty on destruct, " << _images.size()
		      << " entries left");

		while (_images.size()) {
			std::vector<ImageRefEntry>::iterator it =
				_images.end() - 1;
			if (it->get()) {
				P_ERR("delete lost image " << it->getUName());
				delete it->get();
			}
			_images.erase(it);
		}
	}
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
		       std::vector<ImageRefEntry> &images)
{
	if (! file.size()) {
		ref = 0;
		return nullptr;
	}

	std::string real_file(file);
	ImageType image_type = IMAGE_TYPE_TILED;

	// Split image in path # type parts.
	size_t pos = file.rfind('#');
	if (std::string::npos != pos) {
		real_file = file.substr(0, pos);
		image_type = Util::StringToGet(image_type_map,
					       file.substr(pos + 1));
	}

	// Load the image, try load paths if not an absolute image path
	// already.
	PImage *image = nullptr;
	if (real_file[0] == '/') {
		std::string u_real_file(real_file);
		Util::to_upper(u_real_file);
		image = getImageFromPath(real_file, u_real_file, ref, images);
	} else {
		std::vector<std::string>::reverse_iterator it =
			_search_path.rbegin();
		for (; it != _search_path.rend(); ++it) {
			std::string sp_real_file = *it + real_file;
			std::string u_sp_real_file(sp_real_file);
			Util::to_upper(u_sp_real_file);
			image = getImageFromPath(sp_real_file, u_sp_real_file,
						 ref, images);
			if (image) {
				break;
			}
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
ImageHandler::getImageFromPath(const std::string &file,
			       const std::string &u_file,
			       uint &ref,
			       std::vector<ImageRefEntry> &images)
{
	// Check cache for entry.
	std::vector<ImageRefEntry>::iterator it = images.begin();
	for (; it != images.end(); ++it) {
		if (it->getUName() == u_file) {
			ref = it->incRef();
			return it->get();
		}
	}

	// Try to load the image, setup cache only if it succeeds.
	PImage *image;
	try {
		image = new PImage(file);
		images.push_back(ImageRefEntry(u_file, image));
		ref = 1;
	} catch (LoadException&) {
		image = nullptr;
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
	std::string key = Util::to_string(static_cast<void*>(image));
	Util::to_upper(key);
	_images.push_back(ImageRefEntry(key, image));
}

PImage*
ImageHandler::getMappedImage(const std::string &file,
			     const std::string &colormap)
{
	std::string u_colormap(colormap);
	Util::to_upper(u_colormap);

	std::map<std::string, std::map<int, int> >::iterator c_it =
		_color_maps.find(u_colormap);
	if (c_it == _color_maps.end()) {
		// no color map present with that name, return no image
		return nullptr;
	}

	std::map<std::string, std::vector<ImageRefEntry> >::iterator i_it =
		_images_mapped.find(u_colormap);
	if (i_it == _images_mapped.end()) {
		_images_mapped[u_colormap] = std::vector<ImageRefEntry>();
	}

	uint ref;
	PImage *image = getImage(file, ref, _images_mapped[u_colormap]);
	if (ref == 1) {
		// new image, requires color mapping.
		mapColors(image, _color_maps[u_colormap]);
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
		std::map<int, int>::const_iterator it = color_map.find(*p);
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
	std::string u_colormap(colormap);
	Util::to_upper(u_colormap);
	returnImage(image, _images_mapped[u_colormap]);
}

void
ImageHandler::returnImage(PImage *image,
			  std::vector<ImageRefEntry> &images)
{
	std::vector<ImageRefEntry>::iterator it = images.begin();
	for (; it != images.end(); ++it) {
		if (it->get() == image) {
			if (it->decRef() == 0) {
				delete it->get();
				images.erase(it);
			}
			return;
		}
	}

	P_ERR("returned image " << image << " not found in handler");
	delete image;
}

void
ImageHandler::clearColorMaps(void)
{
	_color_maps.clear();
}

void
ImageHandler::addColorMap(const std::string& name,
			  const std::map<int,int>& color_map)
{
	std::string u_name(name);
	Util::to_upper(u_name);
	_color_maps[u_name] = color_map;
}
