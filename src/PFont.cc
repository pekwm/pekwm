//
// PFont.cc for pekwm
// Copyright © 2003-2007 Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
#include <cstring>

#include "PFont.hh"
#include "PScreen.hh"
#include "Util.hh"

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::wstring;

wstring PFont::_trim_string = wstring();
const char *FALLBACK_FONT = "fixed";

// PFont

//! @brief PFont constructor
PFont::PFont(PScreen *scr) :
  _scr(scr),
  _height(0), _ascent(0), _descent(0),
  _offset_x(0), _offset_y(0), _justify(FONT_JUSTIFY_LEFT)
{
  _type = FONT_TYPE_NO;
}

//! @brief PFont destructor
PFont::~PFont(void)
{
}

//! @brief Draws the text on the drawable.
//! @param dest destination Drawable
//! @param x x start position
//! @param y y start position
//! @param text text to draw
//! @param max_chars max nr of chars, defaults to 0 == strlen(text)
//! @param max_width max nr of pixels, defaults to 0 == infinity
//! @param trim_type how to trim title if not enough space, defaults to FONT_TRIM_END
void
PFont::draw(Drawable dest, int x, int y, const std::wstring &text,
            uint max_chars, uint max_width, PFont::TrimType trim_type)
{
  if (! text.size()) {
    cerr << " *** WARNING: trying to draw empty string." << endl;
  }

  uint offset = x, chars = max_chars;
  wstring real_text(text);

  if (max_width > 0) {
    // If max width set, make sure text fits in max_width pixels.
    trim (real_text, trim_type, max_width);

    offset += justify(real_text, max_width, 0,
                      (chars == 0) ? real_text.size() : chars);
    
  } else if (chars == 0) {
    // Just set to complete string.
    chars = real_text.size();
  }

  // Draw shadowed font if x or y offset is specified
  if (_offset_x || _offset_y) {
    drawText(dest, offset + _offset_x, y + _ascent + _offset_y,
             real_text, chars, false); // false as in bg
  }

  // Draw main font
  drawText(dest, offset, y + _ascent, real_text, chars, true); // true as in fg
}

//! @brief Trims the text making it max max_width wide
//! @param text
//! @param trim_type
//! @param max_width
//! @param chars
void
PFont::trim(std::wstring &text, PFont::TrimType trim_type, uint max_width)
{
  if (getWidth(text) > max_width) {
    if ((_trim_string.size() > 0) && (trim_type == FONT_TRIM_MIDDLE)) {
      // Trim middle before end if trim string is set.
      trimMiddle(text, max_width);
    }

    trimEnd(text, max_width);
  }
}

//! @brief Figures how many charachters to have before exceding max_width
void
PFont::trimEnd(std::wstring &text, uint max_width)
{
  for (uint i = text.size(); i > 0; --i) {
    if (getWidth(text, i) <= max_width) {
      text = text.substr(0, i);
      break;
    }
  }
}

//! @brief Replace middle of string with _trim_string making it max_width wide
void
PFont::trimMiddle(std::wstring &text, uint max_width)
{
  // Get max and separator width
  uint max_side = (max_width / 2);
  uint sep_width = getWidth(_trim_string.c_str(),
                            _trim_string.size() / 2 + _trim_string.size() % 2);

  uint pos = 0;
  wstring dest;

  // If the trim string is too large, do nothing and let trimEnd handle this.
  if (max_side <= sep_width) {
    return;
  }
  // Add space for the trim string
  max_side -= sep_width;

  // Get numbers of chars before trim string (..)
  for (uint i = text.size(); i > 0; --i) {
    if (getWidth(text, i) < max_side) {
      pos = i;
      dest.insert(0, text.substr(0, i));
      break;
    }
  }
  // get numbers of chars after ...
  if (pos < text.size()) {
    for (uint i = pos; i < text.size(); ++i) {
      wstring second_part(text.substr(i, text.size() - i));
      if (getWidth(second_part, 0) < max_side) {
        dest.insert(dest.size(), second_part);
        break;
      }
    }

    // Got a char after and before, if not do nothing and trimEnd will handle
    // trimming after this call.
    if (dest.size() > 1) {
      if ((getWidth(dest) + getWidth(_trim_string)) < max_width) {
        dest.insert(pos, _trim_string);
      }

      // Update original string
      text = dest;
    }
  }
}

//! @brief Justifies the string based on _justify property of the Font
uint
PFont::justify(const std::wstring &text, uint max_width,
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

// PFontX11

//! @brief PFontX11 constructor
PFontX11::PFontX11(PScreen *scr) : PFont(scr),
        _font(NULL),
        _gc_fg(None), _gc_bg(None)
{
    _type = FONT_TYPE_X11;
}

//! @brief PFontX11 destructor
PFontX11::~PFontX11(void)
{
    unload();
}

//! @brief Loads the X11 font font_name
bool
PFontX11::load(const std::string &font_name)
{
    unload();

    _font = XLoadQueryFont(_scr->getDpy(), font_name.c_str());
    if (_font == NULL) {
        _font = XLoadQueryFont(_scr->getDpy(), FALLBACK_FONT);
        if (_font == NULL) {
            return false;
        }
    }

    _height = _font->ascent + _font->descent;
    _ascent = _font->ascent;
    _descent = _font->descent;

    // create GC's
    XGCValues gv;

    uint mask = GCFunction|GCFont;
    gv.function = GXcopy;
    gv.font = _font->fid;
    _gc_fg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);
    _gc_bg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);

    return true;
}

//! @brief Unloads font resources
void
PFontX11::unload(void)
{

    if (_font) {
        XFreeFont(_scr->getDpy(), _font);
        _font = NULL;
    }
    if (_gc_fg != None) {
        XFreeGC(_scr->getDpy(), _gc_fg);
        _gc_fg = None;
    }
    if (_gc_bg != None) {
        XFreeGC(_scr->getDpy(), _gc_bg);
        _gc_bg = None;
    }
}

//! @brief Gets the width the text would take using this font
uint
PFontX11::getWidth(const wstring &text, uint max_chars)
{
  if (! text.size()) {
    return 0;
  }

  // Make sure the max_chars has a decent value, if it's less than 1 wich it
  // defaults to set it to the numbers of chars in the string text
  if (! max_chars || (max_chars > text.size())) {
    max_chars = text.size();
  }
  
  uint width = 0;
  if (_font != NULL) {
#ifdef HAVE_XUTF8TEXTENTS
    // If UTF8 support is available convert to UTF-8 charset and not
    // locale encoding.
    string utf8_text(Util::to_utf8_str(text.substr(0, max_chars)));

    XRectangle logical_ret;
    Xutf8TextExtents(_font, utf8_text.c_str(), utf8_text.size(),
                     NULL, &logical_ret);
    width = logical_ret.width;

#else //! HAVE_XUTF8TEXTENTS
    // No UTF8 support, convert to locale encoding.
    string mb_text(Util::to_mb_str(text.substr(0, max_chars)));
    width = XTextWidth(_font, mb_text.c_str(), mb_text.size());
#endif // HAVE_XUTF8TEXTENTS
  }

  return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontX11::drawText(Drawable dest, int x, int y,
                   const std::wstring &text, uint chars, bool fg)
{
  GC gc = fg ? _gc_fg : _gc_bg;

  if (_font && (gc != None)) {
    string mb_text;
    if (chars != 0) {
      mb_text = Util::to_mb_str(text.substr(0, chars));
    } else {
      mb_text = Util::to_mb_str(text);
    }

    XDrawString(_scr->getDpy(), dest, gc, x, y,
                mb_text.c_str(), mb_text.size());
  }
}

//! @brief Sets the color that should be used when drawing
void
PFontX11::setColor(PFont::Color *color)
{
  if (color->hasFg()) {
    XSetForeground(_scr->getDpy(), _gc_fg, color->getFg()->pixel);
  }

  if (color->hasBg()) {
    XSetForeground(_scr->getDpy(), _gc_bg, color->getBg()->pixel);
  }
}

// PFontXmb

#if 0
//! @brief PFontXmb constructor
PFontXmb::PFontXmb(PScreen *scr) : PFont(scr),
        _fontset(NULL),
        _gc_fg(None), _gc_bg(None)
{
    _type = FONT_TYPE_XMB;
}

//! @brief PFontXmb destructor
PFontXmb::~PFontXmb(void)
{
    unload();
}

//! @brief Loads Xmb font font_name
bool
PFontXmb::load(const std::string &font_name)
{
    unload();

    char *def_string;
    char **missing_list, **font_names;
    int missing_count, fnum;
    XFontStruct **fonts;

    string basename(font_name);
    basename.append(",*");

    _fontset =
        XCreateFontSet(_scr->getDpy(), basename.c_str(),
                       &missing_list, &missing_count, &def_string);

    if (_fontset == NULL) {
        return false;
    }

    for (int i = 0; i < missing_count; ++i)
        cerr <<  "PFontXmb::load(): Missing charset" <<  missing_list[i] << endl;

    _ascent = _descent = 0;

    fnum = XFontsOfFontSet (_fontset, &fonts, &font_names);
    for (int i = 0; i < fnum; ++i) {
        if (signed(_ascent) < fonts[i]->ascent)
            _ascent = fonts[i]->ascent;
        if (signed(_descent) < fonts[i]->descent)
            _descent = fonts[i]->descent;
    }

    _height = _ascent + _descent;

    // create GC's
    XGCValues gv;

    uint mask = GCFunction;
    gv.function = GXcopy;
    _gc_fg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);
    _gc_bg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);

    return true;
}

//! @brief Unloads font resources
void
PFontXmb::unload(void)
{
    if (_fontset != NULL) {
        XFreeFontSet(_scr->getDpy(), _fontset);
        _fontset = NULL;
    }
}

//! @brief Gets the width the text would take using this font
uint
PFontXmb::getWidth(const char *text, uint max_chars)
{
    if (text == NULL)
        return 0;

    // make sure the max_chars has a decent value, if it's less than 1 wich it
    // defaults to set it to the numbers of chars in the string text
    if ((max_chars == 0) || (max_chars > strlen(text)))
        max_chars = strlen(text);

    uint width = 0;
    if (_fontset != NULL) {
        XRectangle ink, logical;
        XmbTextExtents(_fontset, text, max_chars, &ink, &logical);
        width = logical.width;
    }

    return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXmb::drawText(Drawable dest, int x, int y, const char *text, uint chars, bool fg)
{
    GC gc = fg ? _gc_fg : _gc_bg;

    if ((_fontset != NULL) && (gc != None)) {
        XmbDrawString(_scr->getDpy(), dest, _fontset, gc, x, y, text, chars);
    }
}

//! @brief Sets the color that should be used when drawing
void
PFontXmb::setColor(PFont::Color *color)
{
    if (color->hasFg()) {
        XSetForeground(_scr->getDpy(), _gc_fg, color->getFg()->pixel);
    }

    if (color->hasBg()) {
        XSetForeground(_scr->getDpy(), _gc_bg, color->getBg()->pixel);
    }
}

#endif // 0

// PFontXft

#ifdef HAVE_XFT

//! @brief PFontXft constructor
PFontXft::PFontXft(PScreen *scr) : PFont(scr),
        _draw(NULL), _font(NULL),
        _cl_fg(NULL), _cl_bg(NULL)
{
    _type = FONT_TYPE_XFT;
    _draw = XftDrawCreate(_scr->getDpy(), _scr->getRoot(),
                          _scr->getVisual()->getXVisual(), _scr->getColormap());
}

//! @brief PFontXft destructor
PFontXft::~PFontXft(void)
{
    XftDrawDestroy(_draw);

    if (_cl_fg != NULL) {
        XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                     _scr->getColormap(), _cl_fg);
    }
    if (_cl_bg != NULL) {
        XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                     _scr->getColormap(), _cl_bg);
    }
}

//! @brief Loads the Xft font font_name
bool
PFontXft::load(const std::string &font_name)
{
    unload();

    _font = XftFontOpenName(_scr->getDpy(), _scr->getScreenNum(),
                            font_name.c_str());
    if (_font == NULL) {
        _font = XftFontOpenXlfd(_scr->getDpy(), _scr->getScreenNum(),
                                font_name.c_str());
    }

    if (_font != NULL) {
        _ascent = _font->ascent;
        _descent = _font->descent;
        _height = _font->height;

        return true;
    }
    return false;
}

//! @brief Unloads font resources
void
PFontXft::unload(void)
{
    if (_font != NULL) {
        XftFontClose(_scr->getDpy(), _font);
        _font = NULL;
    }
}

//! @brief Gets the width the text would take using this font
uint
PFontXft::getWidth(const std::wstring &text, uint max_chars)
{
  if (! text.size()) {
    return 0;
  }

  // Make sure the max_chars has a decent value, if it's less than 1 wich it
  // defaults to set it to the numbers of chars in the string text
  if (! max_chars || (max_chars > text.size())) {
    max_chars = text.size();
  }

  uint width = 0;
  if (_font != NULL) {
    string utf8_text(Util::to_utf8_str(text.substr(0, max_chars)));

    XGlyphInfo extents;
    XftTextExtentsUtf8(_scr->getDpy(), _font,
                       reinterpret_cast<const XftChar8*>(utf8_text.c_str()),
                       utf8_text.size(), &extents);

    width = extents.xOff;
  }

  return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXft::drawText(Drawable dest, int x, int y,
                   const std::wstring &text, uint chars, bool fg)
{
  XftColor *cl = fg ? _cl_fg : _cl_bg;

  if (_font && cl) {
    string utf8_text;
    if (chars != 0) {
      utf8_text = Util::to_utf8_str(text.substr(0, chars));
    } else {
      utf8_text = Util::to_utf8_str(text);
    }

    XftDrawChange(_draw, dest);
    XftDrawStringUtf8(_draw, cl, _font, x, y,
                      reinterpret_cast<const XftChar8*>(utf8_text.c_str()),
                      utf8_text.size());
  }
}

//! @brief Sets the color that should be used when drawing
void
PFontXft::setColor(PFont::Color *color)
{
    if (_cl_fg != NULL) {
        XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                     _scr->getColormap(), _cl_fg);
        _cl_fg = NULL;
    }
    if (_cl_bg != NULL) {
        XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                     _scr->getColormap(), _cl_bg);
        _cl_bg = NULL;
    }

    if (color->hasFg()) {
        _xrender_color.red = color->getFg()->red;
        _xrender_color.green = color->getFg()->green;
        _xrender_color.blue = color->getFg()->blue;
        _xrender_color.alpha = color->getFgAlpha();

        _cl_fg = new XftColor;

        XftColorAllocValue(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                           _scr->getColormap(), &_xrender_color, _cl_fg);
    }

    if (color->hasBg()) {
        _xrender_color.red = color->getBg()->red;
        _xrender_color.green = color->getBg()->green;
        _xrender_color.blue = color->getBg()->blue;
        _xrender_color.alpha = color->getBgAlpha();

        _cl_bg = new XftColor;

        XftColorAllocValue(_scr->getDpy(), _scr->getVisual()->getXVisual(),
                           _scr->getColormap(), &_xrender_color, _cl_bg);
    }
}

#endif // HAVE_XFT
