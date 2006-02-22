//
// CmdDialog.cc for pekwm
// Copyright (C) 2004-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "PWinObj.hh"
#include "PDecor.hh"
#include "CmdDialog.hh"
#include "Config.hh"
#include "PScreen.hh"
#include "PixmapHandler.hh"
#include "KeyGrabber.hh"
#include "ScreenResources.hh"
#include "Workspaces.hh"

#include <list>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::list;

extern "C" {
#include <X11/Xutil.h> // XLookupString
#include <X11/keysym.h>
}

using std::string;

//! @brief CmdDialog constructor
//! @todo Initial size, configurable?
CmdDialog::CmdDialog(Display *dpy, Theme *theme, const std::string &title) : PDecor(dpy, theme, "CMDDIALOG"),
_cmd_data(theme->getCmdDialogData()),
_cmd_wo(NULL), _bg(None),
_wo_ref(NULL),
_pos(0), _buf_off(0), _buf_chars(0)
{
	// PWinObj attributes
	_type = PWinObj::WO_CMD_DIALOG;
	_layer = LAYER_NONE; // hack, goes over LAYER_MENU
	_hidden = true; // don't care about it when changing worskpace etc

	// add action to list, going to be used from exec
	::Action action;
	_ae.action_list.push_back(action);

	titleAdd(&_title);
	titleSetActive(0);
	setTitle(title);

	_cmd_wo = new PWinObj(_dpy);
	XSetWindowAttributes attr;
	attr.override_redirect = false;
	attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		FocusChangeMask|KeyPressMask|KeyReleaseMask;
	_cmd_wo->setWindow(XCreateWindow(_dpy, _window,
										 0, 0, 1, 1, 0,
										 CopyFromParent, InputOutput, CopyFromParent,
										 CWOverrideRedirect|CWEventMask, &attr));

	addChild(_cmd_wo);
	activateChild(_cmd_wo);
	_cmd_wo->mapWindow();

	// setup texture, size etc
	loadTheme();

	Workspaces::instance()->insert(this);
	_wo_list.push_back(this);
}

//! @brief CmdDialog destructor
CmdDialog::~CmdDialog(void)
{
	Workspaces::instance()->remove(this);
	_wo_list.remove(this);

	// remove ourself from the decor manually, no need to reparent and stuff
	_child_list.remove(_cmd_wo);

	if (_cmd_wo != NULL) {
		XDestroyWindow(_dpy, _cmd_wo->getWindow());
		delete _cmd_wo;
	}

	unloadTheme();
}

//! @brief Handles ButtonPress, moving the text cursor
ActionEvent*
CmdDialog::handleButtonPress(XButtonEvent *ev)
{
	if (*_cmd_wo == ev->window) {
		// FIXME: move cursor
		return NULL;
	} else {
		return PDecor::handleButtonPress(ev);
	}
}

//! @brief Handles KeyPress, editing the buffer
ActionEvent*
CmdDialog::handleKeyPress(XKeyEvent *ev)
{
	ActionEvent *c_ae, *ae = NULL;

	if ((c_ae = KeyGrabber::instance()->findAction(ev, _type)) != NULL) {
		list<Action>::iterator it(c_ae->action_list.begin());
		for (; it != c_ae->action_list.end(); ++it) {
			switch (it->getAction()) {
			case CMD_D_INSERT:
				bufAdd(ev);
				break;
			case CMD_D_REMOVE:
				bufRemove();
				break;
			case CMD_D_CLEAR:
				bufClear();
				break;
			case CMD_D_EXEC:
				ae = exec();

				unmapWindow();
				bufClear();
				break;
			case CMD_D_CLOSE:
				unmapWindow();
				bufClear();
				break;
			case CMD_D_COMPLETE:
				break;
			case CMD_D_CURS_NEXT:
				bufChangePos(1);
				break;
			case CMD_D_CURS_PREV:
				bufChangePos(-1);
				break;
			case CMD_D_CURS_BEGIN:
				_pos = 0;
				break;
			case CMD_D_CURS_END:
				_pos = _buf.size();
				break;
			case CMD_D_HIST_NEXT:
				histNext();
				break;
			case CMD_D_HIST_PREV:
				histPrev();
				break;
			case CMD_D_NO_ACTION:
			default:
				// do nothing, shouldn't happen
				break;
			};
		}

		// something ( most likely ) changed, redraw the window
		if (ae == NULL) {
			bufChanged();
			render();
		}
	}

	return ae;
}

//! @brief Handles ExposeEvent, redraw when ev->count == 0
ActionEvent*
CmdDialog::handleExposeEvent(XExposeEvent *ev)
{
	if (ev->count > 0) {
		return NULL;
	}
	render();
	return NULL;
}

//! @brief Maps the CmdDialog center on the PWinObj it executes actions on.
void
CmdDialog::mapCenteredOnWORef(void)
{
	if (!_wo_ref) {
		_wo_ref = PWinObj::getRootPWinObj();
	}

	if (_wo_ref->getType() == PWinObj::WO_CLIENT) {
		_wo_ref = _wo_ref->getParent();
	}

	// Setup data
	_hist_it = _hist_list.end();

	// Make sure position is inside head.
	Geometry head;
	uint head_nr = PScreen::instance()->getNearestHead(_wo_ref->getX() + (_wo_ref->getWidth() / 2),
																										 _wo_ref->getY() + (_wo_ref->getHeight() / 2));
	PScreen::instance()->getHeadInfo(head_nr, head);

	// Make sure X is inside head.
	int new_x = _wo_ref->getX() + (static_cast<int>(_wo_ref->getWidth())
																 - static_cast<int>(_gm.width)) / 2;
	if (new_x < head.x)
		new_x = head.x;
	else if ((new_x + _gm.width) > (head.x + head.width))
		new_x = head.x + head.width - _gm.width;

	// Make sure Y is inside head.
	int new_y = _wo_ref->getY() + (static_cast<int>(_wo_ref->getHeight())
																	- static_cast<int>(_gm.height)) / 2;
	if (new_y < head.y)
		new_y = head.y;
	else if ((new_y + _gm.height) > (head.y + head.height))
		new_y = head.y + head.height - _gm.height;

	move(new_x, new_y);
			 
	PDecor::mapWindowRaised();
}

//! @brief Sets title of decor
void
CmdDialog::setTitle(const std::string &title)
{
	_title.setReal(title);
	_title.setVisible(title);
}

//! @brief Sets background and size
void
CmdDialog::loadTheme(void)
{
	// setup variables
	_cmd_data = _theme->getCmdDialogData();

	// setup size
	resizeChild(PScreen::instance()->getWidth() / 4,
							_cmd_data->getFont()->getHeight() +
							_cmd_data->getPad(DIRECTION_UP) +
							_cmd_data->getPad(DIRECTION_DOWN));

	// get pixmap
	PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();
	pm->returnPixmap(_bg);
	_bg = pm->getPixmap(_cmd_wo->getWidth(), _cmd_wo->getHeight(),
											PScreen::instance()->getDepth(), false);

	// render texture
	_cmd_data->getTexture()->render(_bg, 0, 0,
																	_cmd_wo->getWidth(), _cmd_wo->getHeight());
	_cmd_wo->setBackgroundPixmap(_bg);
	_cmd_wo->clear();
}

//! @brief Frees resources
void
CmdDialog::unloadTheme(void)
{
	ScreenResources::instance()->getPixmapHandler()->returnPixmap(_bg);
}

//! @brief Renders _buf onto _cmd_wo
void
CmdDialog::render(void)
{
	_cmd_wo->clear();

	// draw buf content
	_cmd_data->getFont()->setColor(_cmd_data->getColor());
	_cmd_data->getFont()->draw(_cmd_wo->getWindow(),
														 _cmd_data->getPad(DIRECTION_LEFT),
														 _cmd_data->getPad(DIRECTION_UP),
														 _buf.c_str() + _buf_off, _buf_chars);

	// draw cursor
	uint pos = _cmd_data->getPad(DIRECTION_LEFT);
	if (_pos > 0) {
		pos = _cmd_data->getFont()->getWidth(_buf.c_str() + _buf_off,
																				 _pos - _buf_off) + 1;
	}

	_cmd_data->getFont()->draw(_cmd_wo->getWindow(),
														 pos, _cmd_data->getPad(DIRECTION_UP),
														 "|");
}

//! @brief Parses _buf and tries to generate an ActionEvent
ActionEvent*
CmdDialog::exec(void)
{
	_hist_list.push_back(_buf);
	if (_hist_list.size() > 10) // FIXME: make configurable
		_hist_list.pop_front();

	// Check if it's a valid Action, if not we assume it's a command and try
	// to execute it.
	if (!Config::instance()->parseAction(_buf,	_ae.action_list.front(),
																			 KEYGRABBER_OK)) {
		_ae.action_list.front().setAction(ACTION_EXEC);
		_ae.action_list.front().setParamS(_buf);
	}

	return &_ae;
}

//! @brief
void
CmdDialog::complete(void)
{
}

//! @brief Adds char to buffer
void
CmdDialog::bufAdd(XKeyEvent *ev)
{
	KeySym ks;
	char c_return;

	XLookupString(ev, &c_return, 1, &ks, NULL);

	if (isprint(c_return) != 0) {
		_buf.insert(_buf.begin() + _pos++, c_return);
	}
}

//! @brief Removes char from buffer
void
CmdDialog::bufRemove(void)
{
	if ((_pos > _buf.size()) || (_pos == 0) || (_buf.size() == 0)) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "CmdDialog(" << this << ")::bufRemove()" << endl
				 << " *** _pos: " << _pos << " ,  _buf.size(): " << _buf.size() << endl;
#endif // DEBUG
		return;
	}

	_buf.erase(_buf.begin() + --_pos);
}

//! @brief Clears the buffer, resets status
void
CmdDialog::bufClear(void)
{
	_buf = ""; // old gcc doesn't know about .clear()
	_pos = _buf_off = _buf_chars = 0;
}

//! @brief Moves the marker
void
CmdDialog::bufChangePos(int off)
{
	if ((signed(_pos) + off) < 0) {
		_pos = 0;
	} else if (unsigned(_pos + off) > _buf.size()) {
		_pos = _buf.size();
	} else {
		_pos += off;
	}
}

//! @brief Recalculates, _buf_off and _buf_chars
void
CmdDialog::bufChanged(void)
{
	PFont *font =  _cmd_data->getFont(); // convenience

	// complete string doesn't fit in the window OR
	// we don't fit in the first set
	if ((_pos > 0)
			&& (font->getWidth(_buf.c_str()) > _cmd_wo->getWidth())
			&& (font->getWidth(_buf.c_str(), _pos) > _cmd_wo->getWidth())) {

		// increase position until it all fits
		for (_buf_off = 0; _buf_off < _pos; ++_buf_off) {
			if (font->getWidth(_buf.c_str() + _buf_off, _buf.size() - _buf_off)
					< _cmd_wo->getWidth())
				break;
		}

		_buf_chars = _buf.size() - _buf_off;
	} else {
		_buf_off = 0;
		_buf_chars = _buf.size();
	}
}

//! @brief
void
CmdDialog::histNext(void)
{
	if (_hist_it == _hist_list.end()) {
		return; // nothing to do
	}

	// get next item, if at the end, restore the edit buffer
	++_hist_it;
	if (_hist_it == _hist_list.end()) {
		_buf = _hist_new;
	} else {
		_buf = *_hist_it;
	}

	// move cursor to the end of line
	_pos = _buf.size();
}

//! @brief
void
CmdDialog::histPrev(void)
{
	if (_hist_it == _hist_list.begin()) {
		return; // nothing to do
	}

	// save item so we can restore the edit buffer later
	if (_hist_it == _hist_list.end()) {
		_hist_new = _buf;
	}

	// get prev item
	_buf = *(--_hist_it);

	// move cursor to the end of line
	_pos = _buf.size();
}
