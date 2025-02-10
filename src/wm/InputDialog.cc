//
// InputDialog.cc for pekwm
// Copyright (C) 2009-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <fstream>
#include <algorithm>

#include "Charset.hh"
#include "InputDialog.hh"
#include "KeyGrabber.hh"
#include "X11.hh"
#include "Workspaces.hh"

extern "C" {
#include <X11/Xutil.h>
}

std::map<KeySym, std::string> InputDialog::_keysym_map;

InputBuffer::InputBuffer(void)
	: _pos(0)
{
}

InputBuffer::InputBuffer(const std::string& buf, int pos)
	: _buf(buf)
{
	if (pos == -1) {
		_pos = buf.size();
	} else {
		_pos = pos;
	}
}

InputBuffer::~InputBuffer(void)
{
}

/**
 * Adds str to buffer
 */
void
InputBuffer::add(const std::string& buf)
{
	for (size_t i = 0; i < buf.size(); i++) {
		_buf.insert(_buf.begin() + _pos++, buf[i]);
	}
}

/**
 * Removes char at current position from buffer
 */
void
InputBuffer::remove(void)
{
	if (_pos == 0 || _pos > _buf.size() || _buf.empty()) {
		return;
	}

	Charset::Utf8Iterator it(_buf, _pos);
	--it;
	_buf.erase(it.pos(), _pos - it.pos());
	_pos = it.pos();
}

void
InputBuffer::clear(void)
{
	_buf = "";
	_pos = 0;
}

/**
 * Removes buffer content up until position
 */
void
InputBuffer::killTo()
{
	_buf = _buf.substr(_pos);
	_pos = 0;
}

/**
 * Removes buffer content after position
 */
void
InputBuffer::killFrom()
{
	_buf.resize(_pos);
}

/**
 * Moves the marker
 */
void
InputBuffer::changePos(int off)
{
	Charset::Utf8Iterator it(_buf, _pos);
	if (off > 0) {
		for (; off > 0; off--) {
			++it;
		}
	} else if (off < 0) {
		for (; off < 0; off++) {
			--it;
		}
	}
	_pos = it.pos();
}


/**
 * InputDialog constructor.
 */
InputDialog::InputDialog(const std::string &title)
	: PDecor(None, true, true, "INPUTDIALOG"), PWinObjReference(0),
	  _data(pekwm::theme()->getCmdDialogData()),
	  _buf_off(0),
	  _buf_chars(0),
	  _text_end(0),
	  _cursor_begin(0),
	  _cursor_end(0)
{
	// PWinObj attributes
	setLayer(LAYER_NONE); // hack, goes over LAYER_MENU
	_hidden = true;

	if (! _keysym_map.size()) {
		reloadKeysymMap();
	}

	// Add action to list, going to be used from close and exec
	::Action action;
	_ae.action_list.push_back(action);

	titleAdd(&_title);
	titleSetActive(0);
	setTitle(title);

	_text_wo = new PWinObj(true);

	Window window =
		X11::createWmWindow(_window, 0, 0, 1, 1, InputOutput,
				    ButtonPressMask|ButtonReleaseMask|
				    ButtonMotionMask|FocusChangeMask|
				    KeyPressMask|KeyReleaseMask);
	_text_wo->setWindow(window);

	addChild(_text_wo);
	addChildWindow(_text_wo->getWindow());
	activateChild(_text_wo);
	_text_wo->mapWindow();

	// setup texture, size etc
	InputDialog::loadTheme();

	Workspaces::insert(this);
	woListAdd(this);
	_wo_map[_window] = this;
}

/**
 * InputDialog destructor.
 */
InputDialog::~InputDialog(void)
{
	Workspaces::remove(this);
	_wo_map.erase(_window);
	woListRemove(this);

	// Free resources
	if (_text_wo) {
		_children.erase(std::remove(_children.begin(), _children.end(),
					    _text_wo),
				_children.end());
		removeChildWindow(_text_wo->getWindow());
		X11::destroyWindow(_text_wo->getWindow());
		delete _text_wo;
	}

	InputDialog::unloadTheme();
}

void
InputDialog::reloadKeysymMap(void)
{
	_keysym_map.clear();

	addKeysymToKeysymMap(XK_KP_0, "0");
	addKeysymToKeysymMap(XK_KP_1, "1");
	addKeysymToKeysymMap(XK_KP_2, "2");
	addKeysymToKeysymMap(XK_KP_3, "3");
	addKeysymToKeysymMap(XK_KP_4, "4");
	addKeysymToKeysymMap(XK_KP_5, "5");
	addKeysymToKeysymMap(XK_KP_6, "6");
	addKeysymToKeysymMap(XK_KP_7, "7");
	addKeysymToKeysymMap(XK_KP_8, "8");
	addKeysymToKeysymMap(XK_KP_9, "9");
}

void
InputDialog::addKeysymToKeysymMap(KeySym keysym, const std::string& chr)
{
	Display *dpy = X11::getDpy();

	int keysyms_per_keycode;
	KeyCode keycode = XKeysymToKeycode(dpy, keysym);
	if (! keycode) {
		return;
	}

	KeySym *keysyms =
		XGetKeyboardMapping(dpy, keycode, 1, &keysyms_per_keycode);
	if (keysyms) {
		for (int i = 0; i < keysyms_per_keycode; i++) {
			if (keysyms[i] != NoSymbol) {
				_keysym_map[keysyms[i]] = chr;
			}
		}

		X11::free(keysyms);
	}
}

/**
 * Handles ButtonPress, moving the text cursor
 */
ActionEvent*
InputDialog::handleButtonPress(XButtonEvent *ev)
{
	if (*_text_wo != ev->window || ev->x < 0) {
		return PDecor::handleButtonPress(ev);
	}

	const uint x = static_cast<uint>(ev->x);
	if (x > _text_end) {
		_buf.setPos(_buf_chars);
	} else if (x < static_cast<uint>(_data->getPad(PAD_LEFT))) {
		_buf.setPos(_buf_off);
	} else if (x >= _cursor_begin && x <= _cursor_end) {
		// do nothing, position unchanged
	} else {
		uint off;
		size_t buf_off = _buf_off;
		const char *c_buf = str().c_str() + buf_off;
		if (x > _cursor_end) {
			c_buf += _buf.pos();
			buf_off += _buf.pos();
			off = _cursor_end;
		}  else {
			off = _data->getPad(PAD_LEFT);
		}

		uint last_pos = off;
		Charset::Utf8Iterator it(c_buf, 0);
		for (++it; it.ok(); ++it) {
			uint pos = off
				+ _data->getFont()->getWidth(c_buf, it.pos());
			if (x < pos) {
				uint half_width = (pos - last_pos) / 2;
				if (x < (last_pos + half_width)) {
					--it;
				}
				break;
			}
			last_pos = pos;
		}
		_buf.setPos(it.pos() + buf_off);
	}
	render();
	return nullptr;
}

/**
 * Handles KeyPress, editing the buffer
 */
ActionEvent*
InputDialog::handleKeyPress(XKeyEvent *ev)
{
	bool matched;
	ActionEvent *c_ae;
	ActionEvent *r_ae = nullptr;
	if ((c_ae = pekwm::keyGrabber()->findAction(ev, _type, matched))) {
		ActionEvent::it it(c_ae->action_list.begin());
		for (; it != c_ae->action_list.end(); ++it) {
			ActionEvent *ae = handleAction(ev, *it);
			if (ae) {
				r_ae = ae;
			}
		}

		// something (most likely) changed, redraw the window
		if (! r_ae) {
			bufChanged();
			render();
		}
	}

	return r_ae;
}

ActionEvent*
InputDialog::handleAction(XKeyEvent *ev, const Action &action)
{
	switch (action.getAction()) {
	case INPUT_INSERT:
		bufAdd(ev);
		completeReset();
		return nullptr;
	case INPUT_REMOVE:
		_buf.remove();
		return nullptr;
	case INPUT_CLEAR:
		bufClear();
		return nullptr;
	case INPUT_CLEARFROMCURSOR:
		_buf.killFrom();
		completeReset();
		return nullptr;
	case INPUT_CLEARTOCURSOR:
		_buf.killTo();
		completeReset();
		return nullptr;
	case INPUT_EXEC:
		return exec();
	case INPUT_CLOSE:
		return close();
	case INPUT_COMPLETE:
		complete();
		return nullptr;
	case INPUT_COMPLETE_ABORT:
		completeAbort();
		return nullptr;
	case INPUT_CURS_NEXT:
		_buf.changePos(1);
		completeReset();
		return nullptr;
	case INPUT_CURS_PREV:
		_buf.changePos(-1);
		completeReset();
		return nullptr;
	case INPUT_CURS_BEGIN:
		_buf.setPos(0);
		completeReset();
		return nullptr;
	case INPUT_CURS_END:
		_buf.setPos(_buf.size());
		completeReset();
		return nullptr;
	case INPUT_HIST_NEXT:
		histNext();
		completeReset();
		return nullptr;
	case INPUT_HIST_PREV:
		histPrev();
		completeReset();
		return nullptr;
	case INPUT_NO_ACTION:
	default:
		// do nothing, shouldn't happen
		return nullptr;
	};
}

/**
 * Handles ExposeEvent, redraw when ev->count == 0
 */
ActionEvent*
InputDialog::handleExposeEvent(XExposeEvent *ev)
{
	if (ev->count > 0) {
		return 0;
	}
	render();
	return 0;
}

/**
 * Maps the InputDialog center on the PWinObj it executes actions on.
 *
 * @param buf Buffer content.
 * @param wo_ref PWinObj reference, defaults to 0 which does not update.
 */
void
InputDialog::mapCentered(const std::string &buf, const Geometry &gm,
			 PWinObj *wo_ref)
{
	// Setup data
	_hist_it = _history.end();

	_buf.setBuf(buf);
	_buf.setPos(buf.size());
	bufChanged();

	Geometry head;
	uint head_nr = X11::getNearestHead(gm.x + (gm.width / 2),
					   gm.y + (gm.height / 2));
	X11::getHeadInfo(head_nr, head);

	// Update size (before, as we center) and position
	updateSize(head);
	moveCentered(head, gm);

	// Map and render
	PDecor::mapWindowRaised();
	render();

	giveInputFocus();
}

/**
 * Moves to center of geometry.
 *
 * @param gm Geometry to center on.
 */
void
InputDialog::moveCentered(const Geometry &head, const Geometry &gm)
{
	// Make sure X is inside head.
	int new_x =
		gm.x + (static_cast<int>(gm.width)
			- static_cast<int>(_gm.width)) / 2;
	if (new_x < head.x) {
		new_x = head.x;
	} else if ((new_x + _gm.width) > (head.x + head.width)) {
		new_x = head.x + head.width - _gm.width;
	}

	// Make sure Y is inside head.
	int new_y =
		gm.y + (static_cast<int>(gm.height)
			- static_cast<int>(_gm.height)) / 2;
	if (new_y < head.y) {
		new_y = head.y;
	} else if ((new_y + _gm.height) > (head.y + head.height)) {
		new_y = head.y + head.height - _gm.height;
	}

	// Update position.
	move(new_x, new_y);
}

/**
 * Sets title of decor
 */
void
InputDialog::setTitle(const std::string &title)
{
	_title.setReal(title);
}

/**
 * Maps window, overloaded to refresh content of window after mapping.
 */
void
InputDialog::mapWindow(void)
{
	if (! _mapped) {
		Geometry head;
		X11::getHeadInfo(getHead(), head);

		// Correct size for current head before mapping
		updateSize(head);

		PDecor::mapWindow();
		render();
	}
}

/**
 * Sets background and size
 */
void
InputDialog::loadTheme(void)
{
	Geometry head;
	X11::getHeadInfo(getHead(), head);

	_data = pekwm::theme()->getCmdDialogData();
	updateSize(head);
	updatePixmapSize();
}

/**
 * Frees resources
 */
void
InputDialog::unloadTheme(void)
{
}

/**
 * Renders _buf onto _text_wo
 */
void
InputDialog::render(void)
{
	X11::clearWindow(_text_wo->getWindow());

	// NOTE: this could be optmized by returning the actual width consumed
	//       by the rendering instead of the starting offset avoiding the
	//       extra calls to getWidth.
	PFont *font = _data->getFont();
	font->setColor(_data->getColor());

	uint pos = _data->getPad(PAD_LEFT);
	const char *c_buf = str().c_str() + _buf_off;

	// cursor at the beginning, render before the text
	if (_buf.pos() == 0) {
		pos += renderCursor(pos);
	}
	font->draw(_text_wo, pos, _data->getPad(PAD_UP), c_buf, _buf.pos());

	// cursor in or at the end of the text, render after first part
	if (_buf.pos() != 0) {
		pos += font->getWidth(c_buf, _buf.pos());
		pos += renderCursor(pos);
		if (_buf_chars > _buf.pos()) {
			c_buf += _buf.pos();
			size_t buf_chars = _buf_chars - _buf.pos();
			font->draw(_text_wo, pos, _data->getPad(PAD_UP),
				   c_buf, buf_chars);
			pos += font->getWidth(c_buf, buf_chars);
		}
	}
	_text_end = pos;
}

uint
InputDialog::renderCursor(uint pos)
{
	_data->getFont()->draw(_text_wo, pos, _data->getPad(PAD_UP), "|");
	_cursor_begin = pos;
	uint width = _data->getFont()->getWidth("|", 1);
	_cursor_end = pos + width;
	return width;
}

/**
 * Generates ACTION_CLOSE closing dialog.
 *
 * @return Pointer to ActionEvent.
 */
ActionEvent*
InputDialog::close(void)
{
	_ae.action_list.back().setAction(ACTION_NO);
	return &_ae;
}

void
InputDialog::complete(void)
{
}

void
InputDialog::completeAbort(void)
{
}

void
InputDialog::completeReset(void)
{
}

/**
 * Adds char to buffer
 */
void
InputDialog::bufAdd(XKeyEvent *ev)
{
	KeySym keysym;
	char c_return[64] = {0};
	XLookupString(ev, c_return, sizeof(c_return), &keysym, 0);
	if (_keysym_map.count(keysym)) {
		_buf.add(_keysym_map[keysym]);
	} else {
		_buf.add(c_return);
	}
}


/**
 * Clears the buffer, resets status
 */
void
InputDialog::bufClear(void)
{
	_buf_off = 0;
	_buf_chars = 0;

	_buf.clear();
	completeReset();
}

/**
 * Recalculates, _buf_off and _buf_chars
 */
void
InputDialog::bufChanged(void)
{
	PFont *font =  _data->getFont(); // convenience

	// complete string does not fit in the window OR the first set
	// does not fit
	if (_buf.pos() > 0
	    && (font->getWidth(str()) > _text_wo->getWidth())
	    && (font->getWidth(str(), _buf.pos()) > _text_wo->getWidth())) {

		// increase position until it all fits
		Charset::Utf8Iterator it(_buf.str(), 0);
		for (; it.pos() < _buf.pos(); ++it) {
			if (font->getWidth(_buf.str().c_str() + it.pos(),
					   _buf.size() - it.pos())
			    < _text_wo->getWidth()) {
				break;
			}
		}

		_buf_chars = _buf.size() - it.pos();
	} else {
		_buf_off = 0;
		_buf_chars = _buf.size();
	}
}

/**
 * Sets the buffer to the next item in the history.
 */
void
InputDialog::histNext(void)
{
	if (_hist_it == _history.end()) {
		return; // nothing to do
	}

	// get next item, if at the end, restore the edit buffer
	++_hist_it;
	if (_hist_it == _history.end()) {
		_buf.setBuf(_hist_new);
	} else {
		_buf.setBuf(*_hist_it);
	}

	// move cursor to the end of line
	_buf.setPos(_buf.str().size());
}

/**
 * Sets the buffer to the previous item in the history.
 */
void
InputDialog::histPrev(void)
{
	if (_hist_it == _history.begin()) {
		return; // nothing to do
	}

	// save item so we can restore the edit buffer later
	if (_hist_it == _history.end()) {
		_hist_new = _buf.str();
	}

	// get prev item
	_buf.setBuf(*(--_hist_it));

	// move cursor to the end of line
	_buf.setPos(_buf.size());
}

/**
 * Update command dialog size for view on current head.
 */
void
InputDialog::updateSize(const Geometry &head)
{
	// Resize the child window and update the size depending.
	uint old_width = _gm.width;

	unsigned int width, height;
	getInputSize(head, width, height);
	resizeChild(width, height);

	// If size was updated, replace the texture and recalculate display
	// buffer.
	if (old_width != _gm.width) {
		updatePixmapSize();
		bufChanged();
	}
}

/**
 * Update background pixmap size and redraw.
 */
void
InputDialog::updatePixmapSize(void)
{
	PTexture *tex = _data->getTexture();
	tex->setBackground(_text_wo->getWindow(),
			   0, 0, _text_wo->getWidth(), _text_wo->getHeight());
	X11::clearWindow(_text_wo->getWindow());
}

/**
 * Get size of the text input widget.
 *
 * @param width Fill in width.
 * @param height Fill in height.
 */
void
InputDialog::getInputSize(const Geometry &head,
			  unsigned int &width, unsigned int &height)
{
	width = head.width / 3;
	height = _data->getFont()->getHeight()
		+ _data->getPad(PAD_UP) + _data->getPad(PAD_DOWN);
}

void
InputDialog::addHistory(const std::string& entry, bool unique, uint max_size)
{
	if (unique) {
		addHistoryUnique(entry);
	} else {
		_history.push_back(entry);
	}

	if (_history.size() > max_size) {
		_history.erase(_history.begin());
	}
}

void
InputDialog::addHistoryUnique(const std::string& entry)
{
	std::vector<std::string>::iterator it =
		find(_history.begin(), _history.end(), entry);
	if (it != _history.end()) {
		_history.erase(it);
	}

	_history.push_back(entry);
}

void
InputDialog::loadHistory(const std::string &path)
{
	std::ifstream ifile(path.c_str());
	if (ifile.is_open()) {
		// Update only path if successfully opened.
		std::string mb_line;
		while (ifile.good()) {
			getline(ifile, mb_line);
			if (mb_line.size()) {
				_history.push_back(mb_line);
			}
		}
		ifile.close();
	}
}

void
InputDialog::saveHistory(const std::string &path)
{
	std::ofstream ofile(path.c_str());
	if (ofile.is_open()) {
		std::vector<std::string>::iterator it = _history.begin();
		for (; it != _history.end(); ++it) {
			ofile << *it << "\n";
		}
		ofile.close();
	}
}
