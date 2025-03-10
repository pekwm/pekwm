//
// TextureHandler.cc for pekwm
// Copyright (C) 2004-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh"
#include "TextureHandler.hh"
#include "ThemeUtil.hh"
#include "X11.hh"

#include <iostream>

TextureHandler::TextureHandler(float scale)
	: _scale(scale),
	  _length_min(0)
{
	registerTexture("SOLID", parseSolid);
	registerTexture("SOLIDRAISED", parseSolidRaised);
	registerTexture("LINESHORZ", parseLinesHorz);
	registerTexture("LINESVERT", parseLinesVert);
	registerTexture("IMAGE", parseImage);
	registerTexture("IMAGEMAPPED", parseImageMapped);
	registerTexture("EMPTY", parseEmpty);
	registerTexture(nullptr, nullptr);
}

TextureHandler::~TextureHandler(void)
{
}

/**
 * Add mapping from the given name to a texture parsing fun.
 */
void
TextureHandler::registerTexture(const char *name, parse_fun fun)
{
	size_t name_len = name ? strlen(name) : 0;
	if (_length_min == 0 || (name_len > 0 && name_len < _length_min)) {
		_length_min = name_len;
	}

	Util::StringTo<parse_fun> st = { name, fun };
	if (_texture_types.empty() || _texture_types.back().name != nullptr) {
		_texture_types.push_back(st);
	} else {
		_texture_types.insert(_texture_types.end() - 1, st);
	}

}

/**
 * Gets or creates a PTexture
 */
PTexture*
TextureHandler::getTexture(const std::string &texture)
{
	if (texture.size() < _length_min) {
		// name to short, can not be a valid texture.
		P_TRACE("texture " << texture << " name too short");
		return nullptr;
	}

	// check for already existing entry
	entry_vector::iterator it(_textures.begin());
	for (; it != _textures.end(); ++it) {
		if ((*it)->getScale() == _scale && *(*it) == texture) {
			(*it)->incRef();
			return (*it)->getTexture();
		}
	}

	// parse texture
	PTexture *ptexture = parse(texture);
	if (ptexture) {
		// create new entry
		TextureHandler::Entry *entry =
			new TextureHandler::Entry(_scale, texture, ptexture);
		entry->incRef();
		_textures.push_back(entry);
	}

	return ptexture;
}

/**
 * Add/Increment reference cont for texture.
 *
 *  @return Pointer to texture referenced.
 */
PTexture*
TextureHandler::referenceTexture(PTexture *texture)
{
	// Check for already existing entry
	entry_vector::iterator it(_textures.begin());
	for (; it != _textures.end(); ++it) {
		if ((*it)->getTexture() == texture) {
			(*it)->incRef();
			return texture;
		}
	}

	// Create new entry
	TextureHandler::Entry *entry =
		new TextureHandler::Entry(_scale, "@", texture);
	entry->incRef();
	_textures.push_back(entry);

	return texture;
}

/**
 * Return/free a texture.
 */
void
TextureHandler::returnTexture(PTexture **texture)
{
	bool found = false;

	entry_vector::iterator it(_textures.begin());
	for (; it != _textures.end(); ++it) {
		if ((*it)->getTexture() == *texture) {
			found = true;

			(*it)->decRef();
			if ((*it)->getRef() == 0) {
				delete *it;
				_textures.erase(it);
			}
			break;
		}
	}

	if (! found) {
		delete *texture;
	}
	*texture = nullptr;
}

/**
 * Log all referenced textures as trace messages.
 */
void
TextureHandler::logTextures(const std::string& msg) const
{
	std::ostringstream oss;
	oss << msg << " " << _textures.size() << " texture entries";
	entry_vector::const_iterator it(_textures.begin());
	for (; it != _textures.end(); ++it) {
		oss << std::endl
		    << "  " << (*it)->getRef()
		    << ": " << (*it)->getName();
	}
	P_TRACE(oss.str());
}

/**
 * Parses the string, and creates a texture
 */
PTexture*
TextureHandler::parse(const std::string &texture)
{
	parse_fun fun = nullptr;
	std::vector<std::string> tok;
	if (Util::splitString(texture, tok, " \t")) {
		fun = Util::StringToGet(getTextureTypes(), tok[0]);
	} else {
		fun = Util::StringToGet(getTextureTypes(), texture);
	}

	PTexture *ptexture = nullptr;
	if (fun) {
		if (tok.size() > 1) {
			tok.erase(tok.begin());
		}
		ptexture = fun(_scale, texture, tok);
	}

	if (ptexture && ! ptexture->isOk()) {
		delete ptexture;
		ptexture = nullptr;
	}
	return ptexture;
}

/**
 * Parse and create PTextureSolid
 */
PTexture*
TextureHandler::parseSolid(float scale, const std::string &str,
			   const std::vector<std::string> &tok)
{
	if (tok.size() < 1) {
		USER_WARN("missing parameter to texture Solid");
		return 0;
	}

	PTextureSolid *tex = new PTextureSolid(tok[0]);
	if (tok.size() == 2) {
		parseSize(tex, scale, tok[1]);
	}

	return tex;
}

/**
 * Parse and create PTextureSolidRaised
 */
PTexture*
TextureHandler::parseSolidRaised(float scale, const std::string &str,
				 const std::vector<std::string> &tok)
{
	if (tok.size() < 3) {
		USER_WARN("not enough parameters to texture SolidRaised "
			  "(3 required)");
		return 0;
	}

	size_t i = 0;
	PTextureSolidRaised *tex =
		new PTextureSolidRaised(tok[i], tok[i + 1], tok[i + 2]);
	i += 3;

	// Check if we have line width and offset.
	if (tok.size() > (i + 1)) {
		tex->setLineWidth(parsePixels(scale, tok[i]));
		tex->setLineOff(parsePixels(scale, tok[i + 1]));
		i += 2;
	}
	// Check if have side draw specified.
	if (tok.size() > (i + 3)) {
		tex->setDraw(Util::isTrue(tok[i]),
			     Util::isTrue(tok[i + 1]),
			     Util::isTrue(tok[i + 2]),
			     Util::isTrue(tok[i + 3]));
		i += 4;
	}

	// Check if we have size
	if (tok.size() > i) {
		parseSize(tex, scale, tok[i]);
	}

	return tex;
}


PTexture*
TextureHandler::parseLinesHorz(float scale, const std::string &,
			       const std::vector<std::string> &tok)
{
	return parseLines(scale, true, tok);
}

PTexture*
TextureHandler::parseLinesVert(float scale, const std::string &,
			       const std::vector<std::string> &tok)
{
	return parseLines(scale, false, tok);
}

/**
 * Parse and create PTextureLines
 */
PTexture*
TextureHandler::parseLines(float scale, bool horz,
			   const std::vector<std::string> &tok)
{
	if (tok.size() < 2) {
		USER_WARN("not enough parameters to texture Lines"
			  << (horz ? "Horz" : "Vert") << " (2 required)");
		return 0;
	}

	float line_size;
	bool size_percent;
	try {
		if (tok[0][tok[0].size()-1] == '%') {
			size_percent = true;
			std::string size = tok[0].substr(0, tok[0].size() - 1);
			line_size = std::stof(size) / 100;
		} else {
			size_percent = false;
			line_size = parsePixels(scale, tok[0]);
		}
	} catch (const std::invalid_argument&) {
		return nullptr;
	}

	if (line_size == 0.0) {
		return nullptr;
	}

	std::vector<std::string>::const_iterator cend(tok.end());
	uint width = 0;
	uint height = 0;
	if (X11::parseSize(tok.back(), width, height)) {
		--cend;
	}

	PTexture *tex = new PTextureLines(line_size, size_percent, horz,
					  tok.begin() + 1, cend);
	tex->setWidth(ThemeUtil::scaledPixelValue(scale, width));
	tex->setHeight(ThemeUtil::scaledPixelValue(scale, height));
	return tex;
}

/**
 * Parse Image texture.
 */
PTexture*
TextureHandler::parseImage(float scale, const std::string& str,
			   const std::vector<std::string> &tok)
{
	// 6==strlen("IMAGE ")
	PTextureImage *image = new PTextureImage(str.substr(6), "");
	if (! image->isOk()) {
		size_t pos = str.find_first_not_of(" \t", 6);
		image->setImage(str.substr(pos), "");
	}
	return image;
}

/**
 * Parse Image texture with colormap.
 */
PTexture*
TextureHandler::parseImageMapped(float scale, const std::string& str,
				 const std::vector<std::string> &tok)
{
	// 12==strlen("IMAGEMAPPED ")
	size_t map_start = str.find_first_not_of(" \t", 12);
	size_t map_end = str.find_first_of(" \t", map_start);
	if (map_end == std::string::npos) {
		USER_WARN("not enough parameters to texture ImageMapped "
			  "(2 required)");
		return nullptr;
	}
	size_t image_start = str.find_first_not_of(" \t", map_end + 1);
	if (image_start == std::string::npos) {
		USER_WARN("not enough parameters to texture ImageMapped "
			  "(2 required)");
		return nullptr;
	}
	std::string colormap = str.substr(map_start, map_end - map_start);
	return new PTextureImage(str.substr(image_start), colormap);
}

PTexture*
TextureHandler::parseEmpty(float, const std::string& str,
			   const std::vector<std::string> &tok)
{
	return new PTextureEmpty();
}

/**
 * Parses size parameter, i.e. 10x20
 */
bool
TextureHandler::parseSize(PTexture *tex, float scale, const std::string &size)
{
	uint width, height;
	if (X11::parseSize(size, width, height)) {
		tex->setWidth(ThemeUtil::scaledPixelValue(scale, width));
		tex->setHeight(ThemeUtil::scaledPixelValue(scale, height));
		return true;
	}
	return false;
}


uint
TextureHandler::parsePixels(float scale, const std::string &str)
{
	try {
		return ThemeUtil::scaledPixelValue(scale, std::stoi(str));
	} catch (std::invalid_argument&) {
		return 0;
	}
}
