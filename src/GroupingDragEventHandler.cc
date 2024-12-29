//
// GroupingDragEvenHandler.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "ClientMgr.hh"
#include "Debug.hh"
#include "ActionHandler.hh"
#include "GroupingDragEventHandler.hh"
#include "Workspaces.hh"

GroupingDragEventHandler::GroupingDragEventHandler(Frame* frame,
						   Client *client,
						   int x_root, int y_root,
						   bool behind)
	: _x(x_root),
	  _y(y_root),
	  _init(false),
	  _frame(frame),
	  _client(client),
	  _behind(behind)
{
	pekwm::observerMapping()->addObserver(frame, this, 100);
	pekwm::observerMapping()->addObserver(client, this, 100);
}
GroupingDragEventHandler::~GroupingDragEventHandler(void)
{
	if (_frame) {
		pekwm::observerMapping()->removeObserver(_frame, this);
	}
	if (_client) {
		pekwm::observerMapping()->removeObserver(_client, this);
	}
	stopGroupingDrag();
}

void
GroupingDragEventHandler::notify(Observable *observable,
				 Observation *observation)
{
	if (observation != &PWinObj::pwin_obj_deleted) {
		return;
	}
	if (observable == _frame) {
		P_TRACE("frame " << _frame << " lost while grouping drag");
		_frame = nullptr;
	}
	if (observable == _client) {
		P_TRACE("client " << _client << " lost while grouping drag");
		_client = nullptr;
	}
}

bool
GroupingDragEventHandler::initEventHandler(void)
{
	if (! _frame
	    || ! _client
	    || ! X11::grabPointer(X11::getRoot(),
				  ButtonMotionMask|ButtonReleaseMask,
				  CURSOR_NONE)) {
		return false;
	}

	_name = "Grouping ";
	if (_client->getTitle()->getVisible().size() > 0) {
		_name += _client->getTitle()->getVisible();
	} else {
		_name += "No Name";
	}

	updateStatusWindow(true);

	_init = true;
	return true;
}

EventHandler::Result
GroupingDragEventHandler::handleButtonPressEvent(XButtonEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
GroupingDragEventHandler::handleButtonReleaseEvent(XButtonEvent*)
{
	stopGroupingDrag();

	if (! _frame || ! _client) {
		return EVENT_STOP_SKIP;
	}

	Client *search = nullptr;
	if (ClientMgr::isAllowGrouping()) {
		// If grouping is enabled, search for a client at the
		// destination, to allow de-grouping this is not stopped
		// in the handler init.
		int x, y;
		Window win = X11::translateRootCoordinates(_x, _y, &x, &y);
		search = Client::findClient(win);
	}

	if (search
	    && search->getParent()
	    && (search->getParent() != _frame)
	    && (search->getLayer() > LAYER_BELOW)
	    && (search->getLayer() < LAYER_ONTOP)) {
		// If the current window have focus and the frame exists after
		// removing this client it needs to be redrawn as unfocused
		bool focus = _behind ? false : (_frame->size() > 1);

		_frame->removeChild(_client);

		Frame *frame = static_cast<Frame*>(search->getParent());
		frame->addChild(_client);
		if (! _behind) {
			frame->activateChild(_client);
			frame->giveInputFocus();
		}

		if (focus) {
			_frame->setFocused(false);
		}

	}  else {
		Frame *frame = _frame->detachClient(_client, _x, _y);
		if (frame) {
			frame->giveInputFocus();
		}
	}

	return EventHandler::EVENT_STOP_PROCESSED;
}

EventHandler::Result
GroupingDragEventHandler::handleExposeEvent(XExposeEvent*)
{
	return EventHandler::EVENT_SKIP;
}

EventHandler::Result
GroupingDragEventHandler::handleKeyEvent(XKeyEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
GroupingDragEventHandler::handleMotionNotifyEvent(XMotionEvent *ev)
{
	if (! _frame || ! _client) {
		return stopGroupingDrag();
	}

	// Flush all pointer motion, no need to redraw and redraw.
	X11::removeMotionEvents();

	// Update the position and redraw window
	_x = ev->x_root;
	_y = ev->y_root;
	updateStatusWindow(false);

	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
GroupingDragEventHandler::stopGroupingDrag(void)
{
	if (_init) {
		pekwm::statusWindow()->unmapWindow();
		X11::ungrabPointer();
		_init = false;
	}

	return EventHandler::EVENT_STOP_SKIP;
}

void
GroupingDragEventHandler::updateStatusWindow(bool map)
{
	StatusWindow *sw = pekwm::statusWindow();
	if (map) {
		// draw before map to avoid resize right after the
		// window is mapped.
		sw->draw(_name);
		sw->move(_x, _y);
		sw->mapWindowRaised();
	}
	sw->move(_x, _y);
	sw->draw(_name);
}
