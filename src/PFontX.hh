//
// PFontX.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PFONT_X_HH_
#define _PEKWM_PFONT_X_HH_

#include "PFont.hh"

class PFontX : public PFont {
public:
	PFontX(void);
	virtual ~PFontX(void);

protected:
	virtual std::string toNativeDescr(const PFont::Descr &descr) const;
};


#endif // _PEKWM_PFONT_X_HH_
