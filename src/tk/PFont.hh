//
// PFont.hh for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_HH_
#define _PEKWM_PFONT_HH_

#include "config.h"

#include "Charset.hh"
#include "PSurface.hh"
#include "String.hh"

class PFont
{
public:
	enum Type {
		FONT_TYPE_AUTO,
		FONT_TYPE_X11,
		FONT_TYPE_XMB,
		FONT_TYPE_XFT,
		FONT_TYPE_PANGO,
		FONT_TYPE_PANGO_CAIRO,
		FONT_TYPE_PANGO_XFT,
		FONT_TYPE_EMPTY,
		FONT_TYPE_NO
	};
	enum TrimType {
		FONT_TRIM_END,
		FONT_TRIM_MIDDLE
	};

	class Color {
	public:
		Color(void);
		~Color(void);

		XColor *getFg(void) { return _fg; }
		XColor *getBg(void) { return _bg; }
		void setFg(XColor *xc) { _fg = xc; }
		void setBg(XColor *xc) { _bg = xc; }

		uint getFgAlpha(void) const { return _fg_alpha; }
		uint getBgAlpha(void) const { return _bg_alpha; }
		void setFgAlpha(uint alpha) { _fg_alpha = alpha; }
		void setBgAlpha(uint alpha) { _bg_alpha = alpha; }

		bool hasFg(void) const { return _fg != nullptr; }
		bool hasBg(void) const { return _bg != nullptr; }

	private:
		XColor *_fg;
		XColor *_bg;
		uint _fg_alpha;
		uint _bg_alpha;
	};

	/**
	 * Font family and size specification, part of Descr.
	 */
	class DescrFamily {
	public:
		DescrFamily(const std::string& family, uint size)
			: _familiy(family),
			  _size(size) { }
		~DescrFamily() { }

		const std::string& getFamily() const { return _familiy; }
		uint getSize() const { return _size; }
		void setSize(uint size) { _size = size; }

	private:
		std::string _familiy;
		uint _size;
	};

	/**
	 * Font property, part of Descr.
	 */
	class DescrProp {
	public:
		DescrProp(const std::string& prop, const std::string& value)
			: _prop(prop),
			  _value(value) { }
		~DescrProp() { }

		const std::string& getProp() const { return _prop; }
		const std::string& getValue() const { return _value; }

	private:
		std::string _prop;
		std::string _value;
	};

	/**
	 * Font description, used to specify fonts in the same format between
	 * the different supported backends.
	 */
	class Descr {
	public:
		Descr(const std::string& str, bool use_str);
		~Descr();

		const std::string& str() const;
		bool useStr() const;

		const std::vector<DescrFamily>& getFamilies() const;
		const std::vector<DescrProp>& getProperties() const;

		const DescrProp* getProperty(const std::string& prop) const;

		uint getSize(uint def) const;

	private:
		bool parse(const std::string& str);
		bool parseFamilySize(const std::string& str);
		bool parseProp(const std::string& str);

		void setSizeFromProp();

	private:
		std::string _str;
		bool _use_str;

		std::vector<DescrFamily> _families;
		std::vector<DescrProp> _properties;
	};

	PFont(float scale);
	virtual ~PFont();

	float getScale() const { return _scale; }
	uint getJustify() const { return _justify; }

	inline void setJustify(uint j) { _justify = j; }
	inline void setOffset(uint x, uint y) { _offset_x = x; _offset_y = y; }

	int draw(PSurface *dest, int x, int y, const StringView &text,
		 uint &width_used, uint max_width = 0,
		 PFont::TrimType trim_type = FONT_TRIM_END,
		 bool do_draw = true);

	StringView trim(const StringView &text, TrimType trim_type,
			uint max_width);
	StringView trimEnd(const StringView &text, uint max_width);
	StringView trimMiddle(const StringView &text, uint max_width);

	static void setTrimString(const std::string &trim);

	uint justify(const StringView &text, uint &width_used, int max_width,
		     int padding);

	// virtual interface
	virtual bool load(const PFont::Descr &descr) = 0;
	virtual void unload() = 0;

	virtual uint getWidth(const StringView &text) = 0;
	virtual bool useAscentDescent() const {
		return _ascent > 0 && _descent > 0;
	}
	virtual uint getAscent() const { return _ascent; }
	virtual uint getDescent() const { return _descent; }
	virtual uint getHeight() const { return _height; }

	virtual void setColor(PFont::Color* color) = 0;

protected:
	bool isScaled() const { return _scale != 1.0; }
	virtual std::string toNativeDescr(const PFont::Descr &descr) const = 0;

	float _scale;
	uint _height;
	uint _ascent;
	uint _descent;
	uint _offset_x;
	uint _offset_y;
	uint _justify;

	static std::string _trim_string;
	static std::string _trim_buf;

private:
	StringView fitInWidth(const StringView &text, uint max_width);
	StringView fitInWidthShrink(const StringView &text,
				    Charset::Utf8Iterator &it,
				    uint max_width);
	StringView fitInWidthGrow(const StringView &text,
				  Charset::Utf8Iterator &it,
				  uint max_width);

	StringView fitInWidthEnd(const StringView &text, uint max_width);
	StringView fitInWidthEndShrink(const StringView &text,
				       Charset::Utf8Iterator &it,
				       uint max_width);
	StringView fitInWidthEndGrow(const StringView &text,
				     Charset::Utf8Iterator &it,
				     uint max_width);

	virtual void drawText(PSurface *dest, int x, int y,
			      const StringView &text, bool fg) = 0;

	uint getCWidth();
	uint getTrimStringWidth();

	uint _cwidth;
	uint _trim_width;
};

/**
 * Empty font providing no drawing or size.
 */
class PFontEmpty : public PFont {
public:
	PFontEmpty() : PFont(1.0) { }
	virtual ~PFontEmpty(void) { }

	// virtual interface
	virtual bool load(const PFont::Descr&) { return true; }
	virtual void unload(void) { }
	virtual uint getWidth(const StringView&) { return 0; }
	virtual void setColor(PFont::Color *color) { }

protected:
	virtual std::string toNativeDescr(const PFont::Descr&) const {
		return "EMPTY";
	}

private:
	virtual void drawText(PSurface*, int, int, const StringView&, bool) { }
};

#endif // _PEKWM_PFONT_HH_
