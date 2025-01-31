//
// ResizeEvenHandler.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_RESIZEEVENTHANDLER_HH_
#define _PEKWM_RESIZEEVENTHANDLER_HH_

#include "Client.hh"
#include "EventHandler.hh"
#include "Frame.hh"
#include "Observable.hh"
#include "StatusWindow.hh"

#include "tk/Action.hh"

class ResizeEventHandler : public EventHandler,
			   public Observer
{
public:
	ResizeEventHandler(Config *cfg, Frame* frame, Client *client,
			   bool left, bool x, bool top, bool y);
	virtual ~ResizeEventHandler(void);

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
	handleKeyEvent(XKeyEvent*);
	virtual EventHandler::Result
	handleMotionNotifyEvent(XMotionEvent *ev);

private:
	EventHandler::Result stopResize(void);
	void drawOutline(void);
	void updateStatusWindow(bool map);

	void recalcResizeDrag(int nx, int ny);
	void ensureWithinXSizeHints(uint &width, uint &height);
	void clearMaximizedStatesAfterResize(void);

private:
	Config *_cfg;
	bool _outline;

	int _click_x;
	int _click_y;
	int _offset_x;
	int _offset_y;
	Geometry _gm;
	Geometry _old_gm;

	/** Set to true when the event handler has been initialized. */
	bool _init;
	/** Source frame grouping drag was initiated from. */
	Frame *_frame;
	uint _frame_shaded;
	/** Client currently being grouped, belongs to frame. */
	Client *_client;
	/** Resize started on the left size of the Frame. */
	bool _left;
	/** If true, allow horizontal resize */
	bool _x;
	/** Resize started on the upper size of the Frame. */
	bool _top;
	/** If true, allow vertical resize */
	bool _y;
};

#endif // _PEKWM_RESIZEEVENTHANDLER_HH_
