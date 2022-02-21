//
// GroupingDragEvenHandler.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_GROUPINGDRAGEVENTHANDLER_HH_
#define _PEKWM_GROUPINGDRAGEVENTHANDLER_HH_

#include "Action.hh"
#include "Client.hh"
#include "EventHandler.hh"
#include "Frame.hh"
#include "Observable.hh"
#include "StatusWindow.hh"

class GroupingDragEventHandler : public EventHandler,
				 public Observer
{
public:
	GroupingDragEventHandler(Frame *frame, Client *client,
				 int x_root, int y_root, bool behind);
	virtual ~GroupingDragEventHandler(void);

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
	EventHandler::Result stopGroupingDrag(void);
	void updateStatusWindow(bool map);

private:
	/** Current X root position */
	int _x;
	/** Current Y root position */
	int _y;
	/** Name of client being grouped. */
	std::string _name;

	/** Set to true when the event handler has been initialized. */
	bool _init;
	/** Source frame grouping drag was initiated from. */
	Frame *_frame;
	/** Client currently being grouped, belongs to frame. */
	Client *_client;
	/** If true, client does not get activated in the destination. */
	bool _behind;

};

#endif // _PEKWM_GROUPINGDRAGEVENTHANDLER_HH_
