//
// PFontPango.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_PANGO_HH_
#define _PEKWM_PFONT_PANGO_HH_

#include "config.h"

#ifdef PEKWM_HAVE_PANGO

#include <sstream>

#include "PFont.hh"

extern "C" {
#include <pango/pango.h>
}

class PFontPango : public PFont {
public:
	PFontPango(float scale);
	virtual ~PFontPango();

	// virtual interface
	virtual bool load(const PFont::Descr& descr);
	virtual void unload();

	virtual bool useAscentDescent() const;
	virtual void setColor(PFont::Color* color) = 0;

protected:
	virtual std::string toNativeDescr(const PFont::Descr &descr) const;

	PangoContext* _context;
	PangoFontMap* _font_map;
	PangoFont* _font;
	PangoFontDescription* _font_description;

private:
	void toNativeDescrAddStyle(const PFont::Descr& descr,
				   std::ostringstream& native) const;
	void toNativeDescrAddWeight(const PFont::Descr& descr,
				    std::ostringstream& native) const;
	void toNativeDescrAddStretch(const PFont::Descr& descr,
				     std::ostringstream& native) const;
};

#endif // PEKWM_HAVE_PANGO

#endif // _PEKWM_PFONT_PANGO_HH_
