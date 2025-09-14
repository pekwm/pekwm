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
std::string PFont::_trim_buf = std::string();

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
	  _justify(FONT_JUSTIFY_LEFT),
	  _cwidth(0),
	  _trim_width(0)
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
 * @param do_draw If false, skip actual rendering of font, sets width_used.
 * @return Offset from x where font is drawn.
 */
int
PFont::draw(PSurface *dest, int x, int y, const StringView &text,
	    uint &width_used, uint max_width, PFont::TrimType trim_type,
	    bool do_draw)
{
	width_used = 0;
	if (! text.size()
	    || x > (dest->getX() + static_cast<int>(dest->getWidth()))) {
		return 0;
	}

	uint offset = x;
	if (max_width == 0) {
		max_width = dest->getWidth() - x;
	}

	// Make sure text fits in the available space
	StringView real_text = trim(text, trim_type, max_width);
	if (! real_text.empty()) {
		offset += justify(real_text, width_used, max_width, 0);

		// Draw shadowed font if x or y offset is specified
		if (do_draw && (_offset_x || _offset_y)) {
			drawText(dest, offset + _offset_x, y + _offset_y,
				 real_text, false /* bg */);
		}

		// Draw main font
		if (do_draw) {
			drawText(dest, offset, y, real_text, true /* fg */);
		}
	}

	return offset;
}

/**
 * Trims the text making it max max_width wide
 */
StringView
PFont::trim(const StringView &text, PFont::TrimType trim_type, uint max_width)
{
	if (! text.size()) {
		return text;
	}

	if (getWidth(text) <= max_width) {
		return text;
	}

	if (trim_type == FONT_TRIM_MIDDLE
	    && ! _trim_string.empty()
	    && getTrimStringWidth() < max_width) {
		return trimMiddle(text, max_width);
	}
	return trimEnd(text, max_width);
}

/**
 * Figures how many characters to have before exceeding max_width
 */
StringView
PFont::trimEnd(const StringView &text, uint max_width)
{
	return fitInWidth(text, max_width);
}

/**
 * Replace middle of string with _trim_string making it max_width wide
 */
StringView
PFont::trimMiddle(const StringView &text, uint max_width)
{
	uint max_side = max_width / 2;
	// Add space for the trim string
	max_side -= getTrimStringWidth() / 2;

	StringView before = fitInWidth(text, max_side);
	StringView after = fitInWidthEnd(StringView(text, 0, before.size()),
					 max_side);
	if (before.empty() || after.empty()) {
		return trimEnd(text, max_side);
	}

	_trim_buf = before.str();
	_trim_buf.append(_trim_string);
	_trim_buf.append(*after, after.size());
	return StringView(_trim_buf);
}

void
PFont::setTrimString(const std::string &text)
{
	_trim_string = text;
}

/**
 * Move iterator to position in string so that if fits as many characters as
 * possible inside of max_width.
 */
StringView
PFont::fitInWidth(const StringView &text, uint max_width)
{
	Charset::Utf8Iterator it(text);
	it.seek(max_width / getCWidth());
	uint width = getWidth(StringView(text, it.pos()));
	if (width > max_width) {
		return fitInWidthShrink(text, it, max_width);
	}
	return fitInWidthGrow(text, it, max_width);
}

StringView
PFont::fitInWidthShrink(const StringView &text, Charset::Utf8Iterator &it,
			uint max_width)
{
	for (--it; ! it.begin(); --it) {
		if (getWidth(StringView(text, it.pos())) <= max_width) {
			return StringView(text, it.pos());
		}
	}
	return StringView("", 0);
}

StringView
PFont::fitInWidthGrow(const StringView &text, Charset::Utf8Iterator &it,
			uint max_width)
{
	for (++it; ! it.end(); ++it) {
		if (getWidth(StringView(text, it.pos())) > max_width) {
			return StringView(text, (--it).pos());
		}
	}
	return text;
}

/**
 * Move iterator to position in string so that it fits as many characters as
 * possible inside of max_width, starting from the end of string.
 */
StringView
PFont::fitInWidthEnd(const StringView &text, uint max_width)
{
	Charset::Utf8Iterator it(text);
	size_t pos = max_width / getCWidth();
	if (pos < text.size()) {
		it.seek(text.size() - pos);
	}
	uint width = getWidth(StringView(text, 0, it.pos()));
	if (width > max_width) {
		return fitInWidthEndShrink(text, it, max_width);
	}
	return fitInWidthEndGrow(text, it, max_width);
}

StringView
PFont::fitInWidthEndShrink(const StringView &text, Charset::Utf8Iterator &it,
			   uint max_width)
{
	for (++it; ! it.end(); ++it) {
		if (getWidth(StringView(text, 0, it.pos())) <= max_width) {
			return StringView(text, 0, it.pos());
		}
	}
	return StringView("", 0);
}

StringView
PFont::fitInWidthEndGrow(const StringView &text, Charset::Utf8Iterator &it,
			 uint max_width)
{
	for (--it; ! it.begin(); --it) {
		if (getWidth(StringView(text, 0, it.pos())) > max_width) {
			return StringView(text, 0, (++it).pos());
		}
	}
	return text;
}

/**
 * Justifies the string based on _justify property of the Font
 */
uint
PFont::justify(const StringView &text, uint &width_used, int max_width,
	       int padding)
{
	uint x;
	width_used = getWidth(text);
	int width = static_cast<int>(width_used);

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

/**
 * Get estimated character width.
 */
uint
PFont::getCWidth()
{
	if (_cwidth == 0) {
		_cwidth = getWidth("TeSt - 123") / 10;
	}
	return _cwidth;
}

/**
 * Return width of the separator string.
 */
uint
PFont::getTrimStringWidth()
{
	if (_trim_width == 0) {
		_trim_width = getWidth(_trim_string);
	}
	return _trim_width;
}
