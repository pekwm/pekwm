//
// MoveEvenHandler.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "ActionHandler.hh"
#include "MoveEventHandler.hh"

MoveEventHandler::MoveEventHandler(Config* cfg, PDecor* decor,
                                   int x_root, int y_root)
	: _cfg(cfg),
	  _outline(!cfg->getOpaqueMove()),
	  _show_status_window(cfg->isShowStatusWindow()),
	  _center_on_root(cfg->isShowStatusWindowOnRoot()),
	  _curr_edge(SCREEN_EDGE_NO),
	  _init(false),
	  _decor(decor)
{
	decor->getGeometry(_gm);
	decor->getGeometry(_last_gm);
	_x = x_root - _gm.x;
	_y = y_root - _gm.y;
	_decor_shaded = decor->isShaded() ? _gm.height : 0;

	pekwm::observerMapping()->addObserver(decor, this);
}
MoveEventHandler::~MoveEventHandler(void)
{
	if (_decor) {
		pekwm::observerMapping()->removeObserver(_decor, this);
	}
	stopMove();
}

void
MoveEventHandler::notify(Observable *observable,
                         Observation *observation)
{
	if (observation == &PWinObj::pwin_obj_deleted
	    && observable == _decor) {
		P_TRACE("decor " << _decor << " lost while moving");
		_decor = nullptr;
	}
}

bool
MoveEventHandler::initEventHandler(void)
{
	if (! _decor
	    || ! _decor->allowMove()
	    || ! X11::grabPointer(X11::getRoot(),
				  ButtonMotionMask|ButtonReleaseMask,
				  CURSOR_MOVE)) {
		return false;
	}

	// Grab server to avoid any events causing garbage on the
	// screen as the outline is drawed as an inverted rectangle.
	if (_outline) {
		X11::grabServer();
		drawOutline();
	}
	updateStatusWindow(true);

	_init = true;
	return true;
}

EventHandler::Result
MoveEventHandler::handleButtonPressEvent(XButtonEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
MoveEventHandler::handleButtonReleaseEvent(XButtonEvent*)
{
	stopMove();

	if (_decor) {
		_decor->move(_gm.x, _gm.y);
	}

	return EventHandler::EVENT_STOP_PROCESSED;
}

EventHandler::Result
MoveEventHandler::handleExposeEvent(XExposeEvent*)
{
	return EventHandler::EVENT_SKIP;
}

EventHandler::Result
MoveEventHandler::handleKeyEvent(XKeyEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
MoveEventHandler::handleMotionNotifyEvent(XMotionEvent *ev)
{
	if (! _decor) {
		return stopMove();
	}

	drawOutline(); //clear

	// Flush all pointer motion, no need to redraw and redraw.
	X11::removeMotionEvents();

	_gm.x = ev->x_root - _x;
	_gm.y = ev->y_root - _y;
	PDecor::checkSnap(_decor, _gm);

	if (! _outline && _gm != _last_gm) {
		_last_gm = _gm;
		X11::moveWindow(_decor->getWindow(), _gm.x, _gm.y);
	}

	EdgeType edge = doMoveEdgeFind(ev->x_root, ev->y_root);
	if (edge != _curr_edge) {
		_curr_edge = edge;
		if (edge != SCREEN_EDGE_NO) {
			doMoveEdgeAction(ev, edge);
		}
	}

	updateStatusWindow(false);

	drawOutline();

	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
MoveEventHandler::stopMove(void)
{
	if (_init) {
		if (_show_status_window) {
			pekwm::statusWindow()->unmapWindow();
		}
		if (_outline) {
			drawOutline(); // clear
			X11::ungrabServer(true);
		}
		X11::ungrabPointer();
		_init = false;
	}
	return EventHandler::EVENT_STOP_SKIP;
}

void
MoveEventHandler::drawOutline(void)
{
	if (_outline) {
		PDecor::drawOutline(_gm, _decor_shaded);
	}
}

void
MoveEventHandler::updateStatusWindow(bool map)
{
	if (_show_status_window) {
		char buf[128];
		_decor->getDecorInfo(buf, 128, _gm);

		StatusWindow *sw = pekwm::statusWindow();
		Geometry *sw_gm = _center_on_root ? nullptr : &_gm;
		if (map) {
			// draw before map to avoid resize right after the
			// window is mapped.
			sw->draw(buf, true, sw_gm);
			sw->mapWindowRaised();
		}
		sw->draw(buf, true, sw_gm);
	}
}

EdgeType
MoveEventHandler::doMoveEdgeFind(int x, int y)
{
	EdgeType edge = SCREEN_EDGE_NO;
	if (x <= signed(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_LEFT))) {
		edge = SCREEN_EDGE_LEFT;
	} else if (x >= signed(X11::getWidth() -
			       _cfg->getScreenEdgeSize(SCREEN_EDGE_RIGHT))) {
		edge = SCREEN_EDGE_RIGHT;
	} else if (y <= signed(_cfg->getScreenEdgeSize(SCREEN_EDGE_TOP))) {
		edge = SCREEN_EDGE_TOP;
	} else if (y >= signed(X11::getHeight() -
			       _cfg->getScreenEdgeSize(SCREEN_EDGE_BOTTOM))) {
		edge = SCREEN_EDGE_BOTTOM;
	}
	return edge;
}

void
MoveEventHandler::doMoveEdgeAction(XMotionEvent *ev, EdgeType edge)
{
	uint button = X11::getButtonFromState(ev->state);
	std::vector<ActionEvent> *edge_actions =
		_cfg->getEdgeListFromPosition(edge);
	ActionEvent *ae =
		ActionHandler::findMouseAction(button, ev->state,
					       MOUSE_EVENT_ENTER_MOVING,
					       edge_actions);
	if (ae) {
		ActionPerformedWithOffset ap(_decor, *ae, _x, _y);
		ap.type = ev->type;
		ap.event.motion = ev;
		pekwm::actionHandler()->handleAction(&ap);
	}
}
