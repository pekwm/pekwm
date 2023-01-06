//
// PFontPango.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PFontPango.hh"

extern "C" {
#include <pango/pangoxft.h>
};

PFontPango::PFontPango(void)
{
}

PFontPango::~PFontPango(void)
{
}

bool
PFontPango::load(const std::string& font_name)
{
	return false;
}

void
PFontPango::unload(void)
{
}

uint
PFontPango::getWidth(const std::string& text, uint max_chars)
{
	return 0;
}

void
PFontPango::setColor(PFont::Color* color)
{
}

void
PFontPango::drawText(Drawable dest, int x, int y,
		     const std::string& text, uint chars,
		     bool fg)
{
}
