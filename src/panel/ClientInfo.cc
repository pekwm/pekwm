//
// ClientInfo.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "ClientInfo.hh"
#include "Debug.hh"
#include "X11.hh"

/** empty string, used as default return value. */
static std::string _empty_string;

ClientInfo::ClientInfo(Window window)
	: _window(window),
	  _name(readName()),
	  _gm(readGeometry()),
	  _workspace(readWorkspace()),
	  _icon(PImageIcon::newFromWindow(window))

{
	X11::selectInput(_window, PropertyChangeMask);
	X11Util::readEwmhStates(_window, *this);
}

ClientInfo::~ClientInfo(void)
{
	if (_icon) {
		delete _icon;
	}
}

bool
ClientInfo::handlePropertyNotify(XPropertyEvent *ev)
{
	if (ev->atom == X11::getAtom(NET_WM_NAME)
	    || ev->atom == XA_WM_NAME) {
		_name = readName();
	} else if (ev->atom == X11::getAtom(NET_WM_DESKTOP)) {
		_workspace = readWorkspace();
	} else if (ev->atom == X11::getAtom(STATE)) {
		X11Util::readEwmhStates(_window, *this);
	} else {
		return false;
	}
	return true;
}

std::string
ClientInfo::readName(void)
{
	std::string name;
	if (X11::getUtf8String(_window, NET_WM_VISIBLE_NAME, name)) {
		return name;
	}
	if (X11::getUtf8String(_window, NET_WM_NAME, name)) {
		return name;
	}
	if (X11::getTextProperty(_window, XA_WM_NAME, name)) {
		return name;
	}
	return _empty_string;
}

uint
ClientInfo::readWorkspace(void)
{
	Cardinal workspace;
	if (! X11::getCardinal(_window, NET_WM_DESKTOP, workspace)) {
		P_TRACE("failed to read _NET_WM_DESKTOP on " << _window
			<< " using 0");
		return 0;
	}
	return workspace;
}
