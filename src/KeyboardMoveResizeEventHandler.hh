//
// KeyboardMoveResizeEventHandler.hh for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_KEYBOARDMOVERESIZEEVENTHANDLER_HH_
#define _PEKWM_KEYBOARDMOVERESIZEEVENTHANDLER_HH_

#include "Config.hh"
#include "EventHandler.hh"
#include "Observable.hh"
#include "KeyGrabber.hh"
#include "StatusWindow.hh"

#include "tk/Action.hh"

class KeyboardMoveResizeEventHandler : public EventHandler,
				       public Observer
{
public:
	KeyboardMoveResizeEventHandler(Config* cfg, KeyGrabber *key_grabber,
				       PDecor* decor);
	virtual ~KeyboardMoveResizeEventHandler(void);

	virtual void notify(Observable *observable,
			    Observation *observation);

	virtual bool initEventHandler(void);

	virtual EventHandler::Result
	handleButtonPressEvent(XButtonEvent*);
	virtual EventHandler::Result
	handleButtonReleaseEvent(XButtonEvent*);
	virtual EventHandler::Result
	handleExposeEvent(XExposeEvent*);
	virtual EventHandler::Result
	handleMotionNotifyEvent(XMotionEvent*);
	virtual EventHandler::Result
	handleKeyEvent(XKeyEvent *ev);

	EventHandler::Result
	runMoveResizeAction(const Action& action);

	void drawOutline(void);

	void updateStatusWindow(bool map);

	EventHandler::Result stopMoveResize(void);

private:
	KeyGrabber *_key_grabber;
	bool _outline;
	bool _show_status_window;
	bool _center_on_root;

	Geometry _gm;
	Geometry _old_gm;

	bool _init;
	PDecor *_decor;
	uint _decor_shaded;
};

#endif // _PEKWM_KEYBOARDMOVERESIZEEVENTHANDLER_HH_
