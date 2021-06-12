//
// FontHandler.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_FONTHANDLER_HH_
#define _PEKWM_FONTHANDLER_HH_

#include "config.h"

#include "PFont.hh"
#include "Handler.hh"

#include <map>
#include <string>

//! @brief FontHandler, a caching and font type transparent font handler.
class FontHandler {
public:
	FontHandler(void);
	~FontHandler(void);

	PFont *getFont(const std::string &font);
	void returnFont(PFont *font);

	PFont::Color *getColor(const std::string &color);
	void returnColor(PFont::Color *color);

private:
	void loadColor(const std::string &color, PFont::Color *font_color, bool fg);

private:
	std::vector<HandlerEntry<PFont*> > _fonts;
	std::vector<HandlerEntry<PFont::Color*> > _colors;
};

namespace pekwm
{
	FontHandler* fontHandler();
}

#endif // _PEKWM_FONTHANDLER_HH_
