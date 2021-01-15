//
// FontHandler.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifndef _FONT_HANDLER_HH_
#define _FONT_HANDLER_HH_

#include "PFont.hh"
#include "Handler.hh"
#include "ParseUtil.hh"

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
    void freeColor(PFont::Color *font_color);

private:
    std::vector<HandlerEntry<PFont*> > _fonts;
    std::vector<HandlerEntry<PFont::Color*> > _colours;

    std::map<ParseUtil::Entry, PFont::Type> _map_type;
    std::map<ParseUtil::Entry, FontJustify> _map_justify;
};

namespace pekwm
{
    FontHandler* fontHandler();
};

#endif // _FONT_HANDLER_HH_
