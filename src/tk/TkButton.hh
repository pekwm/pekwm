//
// TkButton.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
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
	virtual ~TkButton();

	virtual void setHeight(uint height);
	virtual bool setState(Window window, ButtonState state);
	virtual bool click(Window window);
	virtual void place(int x, int y, uint, uint tot_height);
	virtual uint widthReq() const;
	virtual uint heightReq(uint) const;
	virtual void render(Render&, PSurface&);

private:
	void render();
	PFont* font() const { return _data->getButtonFont(); }
	void resized();

	stop_fun _stop;
	int _retcode;
	std::string _text;

	PPixmapSurface _background;
	ButtonState _state;
};

#endif // _PEKWM_TK_BUTTON_HH_
