//
// TkButton.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TK_BUTTON_HH_
#define _PEKWM_TK_BUTTON_HH_

#include "TkWidget.hh"
#include "PPixmapSurface.hh"

#include <string>

class TkButton : public TkWidget {
public:
	TkButton(Theme::DialogData* data, PWinObj& parent,
	       stop_fun stop, int retcode, const std::string& text);
	virtual ~TkButton(void);

	virtual bool setState(Window window, ButtonState state);
	virtual bool click(Window window);
	virtual void place(int x, int y, uint, uint tot_height);
	virtual uint widthReq(void) const;
	virtual uint heightReq(uint) const;
	virtual void render(Render&, PSurface&);

private:
	void render(void);

private:
	stop_fun _stop;
	int _retcode;
	std::string _text;
	PFont *_font;

	PPixmapSurface _background;
	ButtonState _state;
};

#endif // _PEKWM_TK_BUTTON_HH_
