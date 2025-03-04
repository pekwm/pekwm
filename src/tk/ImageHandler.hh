//
// ImageHandler.hh for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_IMAGEHANDLER_HH_
#define _PEKWM_IMAGEHANDLER_HH_

#include "config.h"

#include "PImage.hh"
#include "Util.hh"

#include <string>
#include <vector>

class PImage;

/**
 * Reference counted entry.
 */
class ImageRefEntry {
public:
	ImageRefEntry(float scale, const std::string& u_name,
		      PImage* data = nullptr);
	~ImageRefEntry(void);

	PImage* get() { return _data; }
	void set(PImage* data) { _data = data; }

	float getScale() const { return _scale; }
	const std::string& getUName() const { return _u_name; }

	uint getRef(void) const { return _ref; }
	uint incRef(void);
	uint decRef(void);

private:
	float _scale;
	std::string _u_name;
	PImage* _data;
	uint _ref;
};

/**
 * ImageHandler, a caching and image type transparent image handler.
 */
class ImageHandler {
public:
	ImageHandler(float scale);
	~ImageHandler();

	void setDefaultType(ImageType type) { _default_type = type; }
	void setScale(float scale) { _scale = scale; }

	/** Add path entry to the search path. */
	void path_push_back(const std::string &path) {
		_search_path.push_back(path);
	}
	/** Remove newest entry in the search path. */
	void path_pop_back(void) {
		_search_path.pop_back();
	}
	/** Remove all entries from the search path. */
	void path_clear(void) {
		_search_path.clear();
	}

	PImage *getImage(const std::string &file);
	void returnImage(PImage *image);

	void takeOwnership(PImage *image);

	PImage *getMappedImage(const std::string &file,
			       const std::string& colormap);
	void returnMappedImage(PImage *image, const std::string& colormap);

	void clearColorMaps(void);
	void addColorMap(const std::string& name,
			 const std::map<int,int>& color_map);

private:
	PImage *getImage(const std::string &file, uint &ref,
			 std::vector<ImageRefEntry> &images);
	PImage *getImageFromPath(const std::string &file,
				 const std::string &u_file,
				 uint &ref,
				 std::vector<ImageRefEntry> &images);

	void mapColors(PImage *image, const std::map<int,int> &color_map);

	static void returnImage(PImage *image,
				std::vector<ImageRefEntry> &images);
private:
	/** Default image type if none is specified */
	ImageType _default_type;
	/** != 1.0, images are scaled after loading by the given factor. */
	float _scale;

	/** List of directories to search. */
	std::vector<std::string> _search_path;
	/** Loaded images. */
	std::vector<ImageRefEntry> _images;
	/** Loaded images with color mapped data. */
	std::map<std::string, std::vector<ImageRefEntry> > _images_mapped;

	std::map<std::string, std::map<int, int> > _color_maps;
};

namespace pekwm
{
	ImageHandler* imageHandler();
}

#endif // _PEKWM_IMAGEHANDLER_HH_
