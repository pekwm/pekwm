//
// StatusWindow.hh for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_STATUSWINDOW_HH_
#define _PEKWM_STATUSWINDOW_HH_

#include "config.h"

#include "pekwm.hh"
#include "PDecor.hh"

class Theme;

//! @brief Status display window.
class StatusWindow : public PDecor {
public:
	StatusWindow(Theme* theme);
	virtual ~StatusWindow(void);

	void draw(const std::string &text, bool do_center = false,
		  Geometry *gm = 0);

private:
	StatusWindow(const StatusWindow&);
	StatusWindow& operator=(const StatusWindow&);

	// BEGIN - PDecor interface
	virtual void loadTheme(void);
	// END - PDecor interface
	void unloadTheme(void);

	void render(void);

private:
	Theme* _theme;
	PWinObj *_status_wo;
};

namespace pekwm
{
	StatusWindow* statusWindow();
}

#endif // _PEKWM_STATUSWINDOW_HH_
