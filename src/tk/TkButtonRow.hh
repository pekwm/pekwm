//
// TkButtonRow.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TK_BUTTON_ROW_HH_
#define _PEKWM_TK_BUTTON_ROW_HH_

#include "TkWidget.hh"
#include "TkButton.hh"

#include <vector>

class TkButtonRow : public TkWidget
{
public:
	TkButtonRow(Theme::DialogData* data, PWinObj& parent,
		    stop_fun stop, const std::vector<std::string>& options);
	virtual ~TkButtonRow(void);

	virtual bool setState(Window window, ButtonState state);
	virtual bool click(Window window);
	virtual void place(int x, int y, uint width, uint tot_height);
	virtual uint heightReq(uint width) const;
	virtual void render(Render &rend, PSurface &surface);

private:
	std::vector<TkButton*> _buttons;
};

#endif // _PEKWM_TK_BUTTON_ROW_HH_
