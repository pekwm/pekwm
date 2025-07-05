//
// ImageHandler.cc for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "Exception.hh"
#include "ImageHandler.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "PImageSvg.hh"
#include "Util.hh"

extern "C" {
#include <assert.h>
}

static Util::StringTo<ImageType> image_type_map[] =
	{{"TILED", IMAGE_TYPE_TILED},
	 {"SCALED", IMAGE_TYPE_SCALED},
	 {"FIXED", IMAGE_TYPE_FIXED},
	 {nullptr, IMAGE_TYPE_NO}};

ImageRefEntry::ImageRefEntry(float scale, const std::string& u_name,
			     PImage* data)
	: _scale(scale),
	  _u_name(u_name),
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

ImageHandler::ImageHandler(float scale)
	: _default_type(IMAGE_TYPE_TILED),
	  _scale(scale)
{
	clearColorMaps();
}

ImageHandler::~ImageHandler()
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
	ImageType image_type = _default_type;

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
		if (_scale != 1.0 && image_type != IMAGE_TYPE_SCALED) {
			image->scale(_scale, PImage::SCALE_SQUARE);
		}
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
		if (it->getScale() == _scale && it->getUName() == u_file) {
			ref = it->incRef();
			return it->get();
		}
	}

	PImage *image = nullptr;
	ref = 0;

	std::string ext(Util::getFileExt(file));
#ifdef PEKWM_HAVE_IMAGE_SVG
	if (pekwm::ascii_ncase_equal("SVG", ext)) {
		try {
			image = new PImageSvg(file);
		} catch (LoadException &ex) {
		}
	} else
#endif // PEKWM_HAVE_IMAGE_SVG
	{
		uint width, height;
		bool use_alpha;
		uchar *data = load(file, ext, width, height, use_alpha);
		if (data != nullptr) {
			image = new PImageData(data, width, height, use_alpha);
		}
	}

	if (image != nullptr) {
		images.push_back(ImageRefEntry(_scale, u_file, image));
		ref = 1;
	}

	return image;
}

/**
 * Get image data from file.
 *
 * @param file File to load.
 * @param ext Extension of the file to load.
 * @param width Updated with the width of the loaded data on success.
 * @param height Updated with the height of the loaded data on success.
 * @param use_alpha Set to true if data contains alpha.
 * @return Pointer to the loaded data, caller is responsible of delete[] data
 */
uchar*
ImageHandler::load(const std::string &file, const std::string &ext,
		   uint &width, uint &height, bool &use_alpha)
{
	if (! ext.size()) {
		USER_WARN("no file extension on " << file);
		return nullptr;
	}

	uchar *data = nullptr;
#ifdef PEKWM_HAVE_IMAGE_JPEG
	if (pekwm::ascii_ncase_equal(PImageLoaderJpeg::getExt(), ext)) {
		data = PImageLoaderJpeg::load(file, width, height, use_alpha);
	} else
#endif // PEKWM_HAVE_IMAGE_JPEG
#ifdef PEKWM_HAVE_IMAGE_PNG
		if (pekwm::ascii_ncase_equal(PImageLoaderPng::getExt(), ext)) {
			data = PImageLoaderPng::load(file, width, height,
						     use_alpha);
		} else
#endif // PEKWM_HAVE_IMAGE_PNG
#ifdef PEKWM_HAVE_IMAGE_XPM
			if (pekwm::ascii_ncase_equal(PImageLoaderXpm::getExt(),
						     ext)) {
				data = PImageLoaderXpm::load(file,
							     width,
							     height,
							     use_alpha);
			} else
#endif // PEKWM_HAVE_IMAGE_XPM
				{
					// no loader matched
					data = nullptr;
				}

	return data;
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
 * Create a copy of the provided image NOT taking ownership of it, the caller
 * is responsible for deleting the image or calling takeOwnership.
 */
PImage*
ImageHandler::copyToNotOwned(PImage *image)
{
#ifdef PEKWM_HAVE_IMAGE_SVG
	PImageSvg *image_svg = dynamic_cast<PImageSvg*>(image);
	if (image_svg != nullptr) {
		return new PImageSvg(image_svg);
	} else
#endif
	{
		PImageData *image_data = static_cast<PImageData*>(image);
		return new PImageData(image_data);
	}
}

/**
 * Take ownership ower image.
 */
void
ImageHandler::takeOwnership(PImage *image)
{
	std::string key = Util::to_string(static_cast<void*>(image));
	Util::to_upper(key);
	_images.push_back(ImageRefEntry(_scale, key, image));
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
	PImageData *imaged = dynamic_cast<PImageData*>(image);
	if (imaged == nullptr) {
		// unsupported image type
		return;
	}

	int *p = reinterpret_cast<int*>(imaged->getData());
	int num_pixels = imaged->getWidth() * imaged->getHeight();
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
