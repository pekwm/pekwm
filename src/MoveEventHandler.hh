//
// MoveEvenHandler.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_MOVEEVENTHANDLER_HH_
#define _PEKWM_MOVEEVENTHANDLER_HH_

#include "Action.hh"
#include "Config.hh"
#include "EventHandler.hh"
#include "Observable.hh"
#include "StatusWindow.hh"

class MoveEventHandler : public EventHandler,
			 public Observer
{
public:
	MoveEventHandler(Config* cfg, PDecor* decor, int x_root, int y_root);
	virtual ~MoveEventHandler(void);

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
	EventHandler::Result stopMove(void);
	void drawOutline(void);
	void updateStatusWindow(bool map);
	EdgeType doMoveEdgeFind(int x, int y);
	void doMoveEdgeAction(XMotionEvent *ev, EdgeType edge);

private:
	Config *_cfg;

	bool _outline;
	bool _show_status_window;
	bool _center_on_root;
	Geometry _gm;
	Geometry _last_gm;
	EdgeType _curr_edge;

	int _x;
	int _y;

	bool _init;
	PDecor *_decor;
	uint _decor_shaded;
};

#endif // _PEKWM_MOVEEVENTHANDLER_HH_
