//
// ResizeEvenHandler.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "ResizeEventHandler.hh"

ResizeEventHandler::ResizeEventHandler(Config *cfg,
				       Frame* frame, Client *client,
				       bool left, bool x, bool top, bool y)
	: _cfg(cfg),
	  _outline(! pekwm::config()->getOpaqueResize()),
	  _init(false),
	  _frame(frame),
	  _client(client),
	  _left(left),
	  _x(x),
	  _top(top),
	  _y(y)
{
	frame->getGeometry(_gm);
	frame->getGeometry(_old_gm);
	_frame_shaded = frame->isShaded() ? _gm.height : 0;

	pekwm::observerMapping()->addObserver(frame, this, 100);
	pekwm::observerMapping()->addObserver(client, this, 100);
}
ResizeEventHandler::~ResizeEventHandler(void)
{
	if (_frame) {
		pekwm::observerMapping()->removeObserver(_frame, this);
	}
	if (_client) {
		pekwm::observerMapping()->removeObserver(_client, this);
	}
	stopResize();
}

void
ResizeEventHandler::notify(Observable *observable,
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
ResizeEventHandler::initEventHandler(void)
{
	if (! _frame || _frame->isShaded()
	    || ! _client || ! _client->allowResize()
	    || ! X11::grabPointer(X11::getRoot(),
				  ButtonMotionMask|ButtonReleaseMask,
				  CURSOR_RESIZE)) {
		return false;
	}

	_frame->setStateFullscreen(STATE_UNSET);

	// the basepoint of our window
	_click_x = _left ? (_gm.x + _gm.width) : _gm.x;
	_click_y = _top ? (_gm.y + _gm.height) : _gm.y;

	// offset used relative to events
	int pointer_x = _gm.x;
	int pointer_y = _gm.y;
	X11::getMousePosition(pointer_x, pointer_y);
	_offset_x = (_left ? _gm.x : (_gm.x + _gm.width)) - pointer_x;
	_offset_y = (_top ? _gm.y : (_gm.y + _gm.height)) - pointer_y;

	updateStatusWindow(true);

	// grab server to avoid traces of invert draws
	if (_outline) {
		X11::grabServer();
		drawOutline();
	}

	_init = true;
	return true;
}

EventHandler::Result
ResizeEventHandler::handleButtonPressEvent(XButtonEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
ResizeEventHandler::handleButtonReleaseEvent(XButtonEvent*)
{
	return stopResize();
}

EventHandler::Result
ResizeEventHandler::handleExposeEvent(XExposeEvent*)
{
	return EventHandler::EVENT_SKIP;
}

EventHandler::Result
ResizeEventHandler::handleKeyEvent(XKeyEvent*)
{
	// mark as processed disabling wm processing of these events.
	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
ResizeEventHandler::handleMotionNotifyEvent(XMotionEvent *ev)
{
	if (! _frame || ! _client) {
		return stopResize();
	}

	// flush all pointer motion, no need to redraw and redraw.
	X11::removeMotionEvents();

	// clear previous otline before updating the geometry
	drawOutline();

	int new_x = _x ? _offset_x + ev->x : 0;
	int new_y = _y ? _offset_y + ev->y : 0;
	recalcResizeDrag(new_x, new_y);
	updateStatusWindow(false);

	if (! _outline && _old_gm != _gm) {
		// only updated when needed when in opaque mode
		_frame->moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
	}
	_old_gm = _gm;

	drawOutline();

	return EventHandler::EVENT_PROCESSED;
}

EventHandler::Result
ResizeEventHandler::stopResize(void)
{
	if (_init) {
		if (_cfg->isShowStatusWindow()) {
			pekwm::statusWindow()->unmapWindow();
		}
		X11::ungrabPointer();

		drawOutline();

		if (_frame) {
			// Make sure the state isn't set to maximized after
			// the Frame has been resized.
			_frame->clearMaximizedStates();
			if (_outline) {
				_frame->moveResize(_gm.x, _gm.y,
						   _gm.width, _gm.height);
			}
		}

		if (_outline) {
			X11::ungrabServer(true);
		}
		_init = false;
	}

	return EventHandler::EVENT_STOP_SKIP;
}

void
ResizeEventHandler::drawOutline(void)
{
	if (_outline) {
		PDecor::drawOutline(_gm, _frame_shaded);
	}
}

void
ResizeEventHandler::updateStatusWindow(bool map)
{
	if (! _cfg->isShowStatusWindow()) {
		return;
	}

	char buf[128];
	_frame->getDecorInfo(buf, 128, _gm);

	StatusWindow *sw = pekwm::statusWindow();
	Geometry *sw_gm = _cfg->isShowStatusWindowOnRoot() ? nullptr : &_gm;
	if (map) {
		// draw before map to avoid resize after the window is mapped.
		sw->draw(buf, true, sw_gm);
		sw->mapWindowRaised();
	}
	sw->draw(buf, true, sw_gm);
}

/**
 * Updates the width, height of the frame when resizing it.
 */
void
ResizeEventHandler::recalcResizeDrag(int nx, int ny)
{
	if (_left) {
		if (nx >= signed(_click_x - _frame->decorWidth(_frame))) {
			nx = _click_x - _frame->decorWidth(_frame) - 1;
		}
	} else if (nx <= signed(_click_x + _frame->decorWidth(_frame))) {
		nx = _click_x + _frame->decorWidth(_frame) + 1;
	}

	if (_top) {
		if (ny >= signed(_click_y - _frame->decorHeight(_frame))) {
			ny = _click_y - _frame->decorHeight(_frame) - 1;
		}
	} else if (ny <= signed(_click_y + _frame->decorHeight(_frame))) {
		ny = _click_y + _frame->decorHeight(_frame) + 1;
	}

	if (_x) {
		_gm.width = _left ? (_click_x - nx) : (nx - _click_x);
	}
	_gm.width -= _frame->decorWidth(_frame);
	if (_y) {
		_gm.height = _top ? (_click_y - ny) : (ny - _click_y);
	}
	_gm.height -= _frame->decorHeight(_frame);

	_client->getAspectSize(&_gm.width, &_gm.height, _gm.width, _gm.height);
	ensureWithinXSizeHints(_gm.width, _gm.height);

	_gm.width += _frame->decorWidth(_frame);
	_gm.height += _frame->decorHeight(_frame);

	_gm.x = _left ? (_click_x - _gm.width) : _click_x;
	_gm.y = _top ? (_click_y - _gm.height) : _click_y;
}

void
ResizeEventHandler::ensureWithinXSizeHints(uint &width, uint &height)
{

	XSizeHints hints = _client->getActiveSizeHints();
	if (hints.flags & PMinSize) {
		if (signed(width) < hints.min_width) {
			width = hints.min_width;
		}
		if (signed(height) < hints.min_height) {
			height = hints.min_height;
		}
	}

	if (hints.flags & PMaxSize) {
		if (signed(width) > hints.max_width) {
			width = hints.max_width;
		}
		if (signed(height) > hints.max_height) {
			height = hints.max_height;
		}
	}
}
