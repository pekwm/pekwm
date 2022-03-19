//
// WinLayouter.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_WINLAYOUTER_HH_
#define _PEKWM_WINLAYOUTER_HH_

#include "Frame.hh"
#include "X11.hh"

#include <string>
#include <vector>

class WinLayouter {
public:
	WinLayouter(void) { }
	virtual ~WinLayouter(void) { }

	virtual bool layout(PWinObj *wo, Window parent, const Geometry &gm,
			    int ptr_x, int ptr_y)=0;
};

WinLayouter *WinLayouterFactory(std::string name);

#endif // _PEKWM_WINLAYOUTER_HH_
