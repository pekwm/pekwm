//
// PFont.cc for pekwm
// Copyright (C) 2003-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>
#include <cstring>

#include "Charset.hh"
#include "Debug.hh"
#include "PFont.hh"
#include "Util.hh"
#include "X11.hh"

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
	if (_fg) {
		X11::returnColor(_fg);
	}
	if (_bg) {
		X11::returnColor(_bg);
	}
}

// PFont

//! @brief PFont constructor
PFont::PFont(void) :
	_height(0), _ascent(0), _descent(0),
	_offset_x(0), _offset_y(0), _justify(FONT_JUSTIFY_LEFT)
{
}

//! @brief PFont destructor
PFont::~PFont(void)
{
	PFont::unload();
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
	if (! text.size()) {
		return 0;
	}

	uint offset = x, chars = max_chars;
	std::string real_text(text);

	if (max_width > 0) {
		// If max width set, make sure text fits in max_width pixels.
		trim(real_text, trim_type, max_width);

		offset += justify(real_text, max_width, 0,
				  (chars == 0) ? real_text.size() : chars);
	} else if (chars == 0) {
		// Just set to complete string.
		chars = real_text.size();
	}

	// Draw shadowed font if x or y offset is specified
	if (_offset_x || _offset_y) {
		drawText(dest, offset + _offset_x, y + _ascent + _offset_y,
			 real_text, chars, false /* bg */);
	}

	// Draw main font
	drawText(dest, offset, y + _ascent, real_text, chars, true /* fg */);

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
PFont::setTrimString(const std::string &text) {
	_trim_string = text;
}

//! @brief Justifies the string based on _justify property of the Font
uint
PFont::justify(const std::string &text, uint max_width,
	       uint padding, uint chars)
{
	uint x;
	uint width = getWidth(text, chars);

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
