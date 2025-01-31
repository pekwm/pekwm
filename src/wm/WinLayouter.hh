//
// WinLayouter.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_WINLAYOUTER_HH_
#define _PEKWM_WINLAYOUTER_HH_

#include "tk/PWinObj.hh"

#include <string>

enum WinLayouterType {
	WIN_LAYOUTER_SMART = (1 << 0),
	WIN_LAYOUTER_CENTERED = (1 << 1),
	WIN_LAYOUTER_CENTEREDONPARENT = (1 << 2),
	WIN_LAYOUTER_MOUSENOTUNDER = (1 << 3),
	WIN_LAYOUTER_MOUSECENTERED = (1 << 4),
	WIN_LAYOUTER_MOUSECTOPLEFT = (1 << 5),
	WIN_LAYOUTER_NO = (1 << 6)
};

#define WIN_LAYOUTER_MASK 0x3f
#define WIN_LAYOUTER_SHIFT 6
#define WIN_LAYOUTER_MASK_NUM 5

class WinLayouter {
public:
	WinLayouter(enum WinLayouterType type)
		: _type(type)
	{
	}
	virtual ~WinLayouter() { }

	enum WinLayouterType getType() const { return _type; }

	virtual bool layout(PWinObj *wo, Window parent, const Geometry &gm,
			    int ptr_x, int ptr_y)=0;

private:
	enum WinLayouterType _type;
};

enum WinLayouterType win_layouter_type_from_string(const std::string &name);

WinLayouter *mkWinLayouter(const std::string &name);
WinLayouter *mkWinLayouter(enum WinLayouterType type);

#endif // _PEKWM_WINLAYOUTER_HH_
