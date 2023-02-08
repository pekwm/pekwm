//
// FocusToggleEventHandler.hh for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_FOCUSTOGGLEEVENTHANDLER_HH_
#define _PEKWM_FOCUSTOGGLEEVENTHANDLER_HH_

#include "Config.hh"
#include "EventHandler.hh"
#include "Frame.hh"
#include "Observable.hh"
#include "PMenu.hh"

#include "tk/Action.hh"

class FocusToggleEventHandler : public EventHandler,
				public Observer
{
public:
	FocusToggleEventHandler(Config* cfg, uint button, uint raise, int off,
				bool show_iconified, bool mru);
	virtual ~FocusToggleEventHandler(void);

	virtual void notify(Observable *observable,
			    Observation *observation);
	virtual bool initEventHandler(void);

	virtual EventHandler::Result
	handleButtonPressEvent(XButtonEvent*);
	virtual EventHandler::Result
	handleButtonReleaseEvent(XButtonEvent*);
	virtual EventHandler::Result
	handleExposeEvent(XExposeEvent *ev);
	virtual EventHandler::Result
	handleMotionNotifyEvent(XMotionEvent*);
	virtual EventHandler::Result
	handleKeyEvent(XKeyEvent *ev);

private:

	EventHandler::Result stop(void);
	void setFocusedWo(PWinObj *fo_wo);
	PMenu* createNextPrevMenu(void);
	bool createMenuInclude(Frame *frame, bool show_iconified);

private:
	Config *_cfg;
	uint _button;
	uint _raise;
	int _off;
	bool _show_iconified;
	bool _mru;

	PMenu *_menu;

	PWinObj *_fo_wo;
	bool _was_iconified;
};

#endif // _PEKWM_FOCUSTOGGLEEVENTHANDLER_HH_
