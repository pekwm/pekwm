//
// pekwm_panel.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_HH_
#define _PEKWM_PANEL_HH_

#include <string>

#include "pekwm.hh"

#define DEFAULT_PLACEMENT PANEL_TOP

typedef void(*fdFun)(int fd, void *opaque);

/**
 * Client state, used for selecting correct theme data for the client
 * list.
 */
enum ClientState {
	CLIENT_STATE_FOCUSED,
	CLIENT_STATE_UNFOCUSED,
	CLIENT_STATE_ICONIFIED,
	CLIENT_STATE_NO
};

/**
 * Widget size unit.
 */
enum WidgetUnit {
	WIDGET_UNIT_PIXELS,
	WIDGET_UNIT_PERCENT,
	WIDGET_UNIT_REQUIRED,
	WIDGET_UNIT_REST,
	WIDGET_UNIT_TEXT_WIDTH
};

/**
 * Panel placement.
 */
enum PanelPlacement {
	PANEL_TOP,
	PANEL_BOTTOM
};

/**
 * Size request for widget.
 */
class SizeReq {
public:
	SizeReq(const std::string &text)
		: _unit(WIDGET_UNIT_TEXT_WIDTH),
		  _size(0),
		  _text(text)
	{
	}

	SizeReq(const SizeReq& size_req)
		: _unit(size_req._unit),
		  _size(size_req._size)
	{
		_text = size_req._text;
	}

	SizeReq(WidgetUnit unit, uint size)
		: _unit(unit),
		  _size(size)
	{
	}

	enum WidgetUnit getUnit(void) const { return _unit; }
	uint getSize(void) const { return _size; }
	const std::string& getText(void) const { return _text; }

private:
	WidgetUnit _unit;
	uint _size;
	std::string _text;
};

#endif // _PEKWM_PANEL_HH_
