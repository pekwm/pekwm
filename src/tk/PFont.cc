//
// PFont.cc for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "Charset.hh"
#include "Debug.hh"
#include "PFont.hh"
#include "Util.hh"
#include "pekwm_types.hh"

extern "C" {
#include <string.h>
}

std::string PFont::_trim_string = std::string();

// PFont::Color

PFont::Color::Color(void)
	: _fg(nullptr),
	  _bg(nullptr),
	  _fg_alpha(65535),
	  _bg_alpha(65535)
{
}

PFont::Color::~Color(void)
{
	X11::returnColor(_fg);
	X11::returnColor(_bg);
}

// PFont::Descr

PFont::Descr::Descr(const std::string& str, bool use_str)
	: _str(str),
	  _use_str(use_str)
{
	parse(str);
}

PFont::Descr::~Descr()
{
}

const std::string&
PFont::Descr::str() const
{
	return _str;
}

bool
PFont::Descr::useStr() const
{
	return _use_str;
}


const std::vector<PFont::DescrFamily>&
PFont::Descr::getFamilies() const
{
	return _families;
}

const std::vector<PFont::DescrProp>&
PFont::Descr::getProperties() const
{
	return _properties;
}

const PFont::DescrProp*
PFont::Descr::getProperty(const std::string& prop) const
{
	std::vector<PFont::DescrProp>::const_iterator it = _properties.begin();
	for (; it != _properties.end(); ++it) {
		if (pekwm::ascii_ncase_equal(it->getProp(), prop)) {
			return &(*it);
		}
	}
	return nullptr;
}

/**
 * Get size from description, falling back to size if missing or invalid.
 */
uint
PFont::Descr::getSize(uint size) const
{
	const PFont::DescrProp* prop = getProperty("size");
	if (prop != nullptr) {
		try {
			size = std::stoi(prop->getValue());
		} catch (std::invalid_argument&) { }
	}
	return size;
}

/**
 * Parse font description, uses fontconfig style:
 *
 * <familiy>-<point size>[,<family>-<point-size>]:<p1>=<v1>:<p2>=<v2>
 *
 * All is optional, family, size and properties. Family Size pairs can
 * repeat and are separated by ,.
 *
 */
bool
PFont::Descr::parse(const std::string& str)
{
	std::string families_str, props_str;
	std::string::size_type props_start = str.find_first_of(':');
	if (props_start == std::string::npos) {
		families_str = str;
	} else {
		families_str = str.substr(0, props_start);
		props_str = str.substr(props_start + 1);
	}

	bool ok = true;
	std::vector<std::string>::iterator it;

	std::vector<std::string> families;
	Util::splitString(families_str, families, ",", 0, false, '\\');
	for (it = families.begin(); it != families.end(); ++it) {
		if (! parseFamilySize(*it)) {
			ok = false;
		}
	}

	std::vector<std::string> props;
	Util::splitString(props_str, props, ":", 0, false, '\\');
	for (it = props.begin(); it != props.end(); ++it) {
		if (! parseProp(*it)) {
			ok = false;
		}
	}

	// Size can be specified both using -size and :size= property, set the
	// property size on the families without a size set.
	setSizeFromProp();

	return ok;
}

bool
PFont::Descr::parseFamilySize(const std::string& str)
{
	std::string family;
	uint size = 0;

	std::string::size_type pos = str.find_first_of('-');
	if (pos == std::string::npos) {
		family = str;
	} else {
		family = str.substr(0, pos);
		try {
			size = std::stoi(str.substr(pos + 1));
		} catch (std::invalid_argument&) {
			return false;
		}
	}

	_families.push_back(PFont::DescrFamily(family, size));
	return true;
}

bool
PFont::Descr::parseProp(const std::string& str)
{
	std::string::size_type pos = str.find_first_of('=');
	if (pos == std::string::npos) {
		return false;
	}

	_properties.push_back(PFont::DescrProp(str.substr(0, pos),
					       str.substr(pos + 1)));

	return true;
}

void
PFont::Descr::setSizeFromProp()
{
	const PFont::DescrProp *prop = getProperty("size");
	if (prop == nullptr) {
		return;
	}

	uint size;
	try {
		size = std::stoi(prop->getValue());
	} catch (std::invalid_argument&) {
		return;
	}

	std::vector<PFont::DescrFamily>::iterator it(_families.begin());
	for (; it != _families.end(); ++it) {
		if (it->getSize() == 0) {
			it->setSize(size);
		}
	}
}

// PFont

PFont::PFont(float scale)
	: _scale(scale),
	  _height(0),
	  _ascent(0),
	  _descent(0),
	  _offset_x(0),
	  _offset_y(0),
	  _justify(FONT_JUSTIFY_LEFT)
{
}

PFont::~PFont()
{
}

/**
 * Draws the text on the drawable.
 *
 * @param dest destination PSurface
 * @param x x start position
 * @param y y start position
 * @param text text to draw
 * @param max_chars max nr of chars, defaults to 0 == strlen(text)
 * @param max_width max nr of pixels, defaults to 0 == infinity
 * @param trim_type how to trim title if not enough space, defaults
 *        to FONT_TRIM_END
 */
int
PFont::draw(PSurface *dest, int x, int y, const std::string &text,
	    uint max_chars, uint max_width, PFont::TrimType trim_type)
{
	if (! text.size()
	    || x > (dest->getX() + static_cast<int>(dest->getWidth()))) {
		return 0;
	}

	uint offset = x;
	if (max_chars == 0 || max_chars > text.size()) {
		max_chars = text.size();
	}
	std::string real_text(text, 0, max_chars);
	if (max_width == 0) {
		max_width = dest->getWidth() - x;
	}
	// Make sure text fits in the available space
	trim(real_text, trim_type, max_width);
	offset += justify(real_text, max_width, 0);

	// Draw shadowed font if x or y offset is specified
	if (_offset_x || _offset_y) {
		drawText(dest, offset + _offset_x, y + _offset_y,
			 real_text, real_text.size(), false /* bg */);
	}

	// Draw main font
	drawText(dest, offset, y, real_text, real_text.size(),
		 true /* fg */);

	return offset;
}

/**
 * Trims the text making it max max_width wide
 */
void
PFont::trim(std::string &text, PFont::TrimType trim_type, uint max_width)
{
	if (! text.size()) {
		return;
	}

	if (getWidth(text) > max_width) {
		if (_trim_string.size() > 0
		    && trim_type == FONT_TRIM_MIDDLE
		    && trimMiddle(text, max_width)) {
			return;
		}

		trimEnd(text, max_width);
	}
}

/**
 * Figures how many charachters to have before exceding max_width
 */
void
PFont::trimEnd(std::string &text, uint max_width)
{
	Charset::Utf8Iterator it(text, text.size());
	--it;
	--it;
	for (; ! it.begin(); --it) {
		if (getWidth(text, it.pos()) <= max_width) {
			text.resize(it.pos());
			return;
		}
	}
	text = "";
}

/**
 * Replace middle of string with _trim_string making it max_width wide
 */
bool
PFont::trimMiddle(std::string &text, uint max_width)
{
	bool trimmed = false;

	// Get max and separator width
	uint max_side = (max_width / 2);
	uint sep_width = getWidth(_trim_string);

	uint pos = 0;
	std::string dest;

	// If the trim string is too large, do nothing and let trimEnd handle
	// this.
	if (sep_width > max_width) {
		return false;
	}
	// Add space for the trim string
	max_side -= sep_width / 2;

	// Get numbers of chars before trim string (..)
	Charset::Utf8Iterator it(text, text.size());
	for (--it; ! it.begin(); --it) {
		if (getWidth(text, it.pos()) <= max_side) {
			pos = it.pos();
			dest.insert(0, text.substr(0, it.pos()));
			break;
		}
	}

	// get numbers of chars after ...
	if (pos < text.size()) {
		for (++it; ! it.end(); ++it) {
			std::string second_part(text.substr(it.pos(),
						text.size() - it.pos()));
			if (getWidth(second_part, 0) <= max_side) {
				dest.insert(dest.size(), second_part);
				break;
			}
		}

		// Got a char after and before, if not do nothing and trimEnd
		// will handle trimming after this call.
		if (dest.size() > 1) {
			if ((getWidth(dest) + getWidth(_trim_string))
			    <= max_width) {
				dest.insert(pos, _trim_string);
				trimmed = true;
			}

			// Update original string
			text = dest;
		}
	}

	return trimmed;
}

void
PFont::setTrimString(const std::string &text)
{
	_trim_string = text;
}

/**
 * Justifies the string based on _justify property of the Font
 */
uint
PFont::justify(const std::string &text, int max_width, int padding)
{
	uint x;
	int width = static_cast<int>(getWidth(text));

	switch(_justify) {
	case FONT_JUSTIFY_CENTER:
		x = (max_width - width) / 2;
		break;
	case FONT_JUSTIFY_RIGHT:
		x = max_width - width - padding;
		break;
	default:
		x = padding;
		break;
	}

	return x;
}
