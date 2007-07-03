//
// PFont.hh for pekwm
// Copyright © 2003-2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifndef _PFONT_HH_
#define _PFONT_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <string>
#ifdef HAVE_LIMITS
#include <limits>
#endif // HAVE_LIMITS

extern "C" {
#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif // HAVE_XFT
}

#include "pekwm.hh"

class PScreen;

class PFont
{
public:
    enum Type {
        FONT_TYPE_X11, FONT_TYPE_XFT, FONT_TYPE_XMB, FONT_TYPE_NO
    };
    enum TrimType {
        FONT_TRIM_END, FONT_TRIM_MIDDLE
    };

    class Color {
    public:
      Color(void) : _has_fg(false), _has_bg(false),
                    _fg_alpha(65535), _bg_alpha(65535)
                   { }
      ~Color(void) { }

        inline XColor *getFg(void) { return _fg; }
        inline XColor *getBg(void) { return _bg; }
        inline void setFg(XColor *xc) { _fg = xc; }
        inline void setBg(XColor *xc) { _bg = xc; }

        inline uint getFgAlpha(void) const { return _fg_alpha; }
        inline uint getBgAlpha(void) const { return _bg_alpha; }
        inline void setFgAlpha(uint alpha) { _fg_alpha = alpha; }
        inline void setBgAlpha(uint alpha) { _bg_alpha = alpha; }

        inline bool hasFg(void) const { return _has_fg; }
        inline bool hasBg(void) const { return _has_bg; }
        inline void setHasFg(bool f) { _has_fg = f; }
        inline void setHasBg(bool b) { _has_bg = b; }

    private:
        XColor *_fg, *_bg;
        bool _has_fg, _has_bg;

        uint _fg_alpha, _bg_alpha;
    };

    PFont(PScreen *scr);
    virtual ~PFont(void);

    inline Type getType(void) const { return _type; }
    inline uint getJustify(void) const { return _justify; }

    inline void setJustify(uint j) { _justify = j; }
    inline void setOffset(uint x, uint y) { _offset_x = x; _offset_y = y; }

  void draw(Drawable dest, int x, int y, const std::wstring &text,
            uint max_chars = 0, uint max_width = 0,
            PFont::TrimType trim_type = FONT_TRIM_END);

  void trim(std::wstring &text, TrimType trim_type, uint max_width);
  void trimEnd(std::wstring &text, uint max_width);
  void trimMiddle(std::wstring &text, uint max_width);

  static void setTrimString(const std::wstring &trim) { _trim_string = trim; }

  uint justify(const std::wstring &text, uint max_width,
               uint padding, uint chars);

  // virtual interface
  virtual bool load(const std::string &font_name) { return true; }
  virtual void unload(void) { }

  virtual uint getWidth(const std::wstring &text, uint max_chars = 0)  {
    return 0;
  }
  virtual uint getHeight(void)  { return _height; }

    virtual void setColor(PFont::Color *color)  { }

private:
  virtual void drawText(Drawable dest, int x, int y, const std::wstring &text,
                        uint chars, bool fg) { }

protected:
  PScreen *_scr;
  Type _type;

  uint _height, _ascent, _descent;
  uint _offset_x, _offset_y, _justify;

  static std::wstring _trim_string;
};

class PFontX11 : public PFont {
public:
  PFontX11(PScreen *scr);
  virtual ~PFontX11(void);

  // virtual interface
  virtual bool load(const std::string &name);
  virtual void unload(void);

  virtual uint getWidth(const std::wstring &text, uint max_chars = 0);

  virtual void setColor(PFont::Color *color);

private:
  virtual void drawText(Drawable dest, int x, int y, const std::wstring &text,
                        uint chars, bool fg);

private:
  XFontStruct *_font;
  GC _gc_fg, _gc_bg;
};

class PFontXmb : public PFont {
public:
  PFontXmb(PScreen *scr);
  virtual ~PFontXmb(void);

  // virtual interface
  virtual bool load(const std::string &name);
  virtual void unload(void);

  virtual uint getWidth(const std::wstring &text, uint max_chars = 0);

  virtual void setColor(PFont::Color *color);

private:
  virtual void drawText(Drawable dest, int x, int y, const std::wstring &text,
                        uint chars, bool fg);

private:
  XFontSet _fontset;
  GC _gc_fg, _gc_bg;
};

#ifdef HAVE_XFT
class PFontXft : public PFont {
public:
  PFontXft(PScreen *scr);
  virtual ~PFontXft(void);

  // virtual interface
  virtual bool load(const std::string &font_name);
  virtual void unload(void);

  virtual uint getWidth(const std::wstring &text, uint max_chars = 0);

  virtual void setColor(PFont::Color *color);

private:
  virtual void drawText(Drawable dest, int x, int y, const std::wstring &text,
                        uint chars, bool fg);

private:
  XftDraw *_draw;
  XftFont *_font;
  XftColor *_cl_fg, *_cl_bg;

  XRenderColor _xrender_color;
};
#endif // HAVE_XFT

#endif // _PFONT_HH_
