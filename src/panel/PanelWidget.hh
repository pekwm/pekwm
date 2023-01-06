//
// PanelWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_PANEL_WIDGET_HH_
#define _PEKWM_PANEL_PANEL_WIDGET_HH_

#include <string>

#include "pekwm_panel.hh"
#include "PanelTheme.hh"
#include "PWinObj.hh"
#include "X11.hh"

/**
 * Base class for all widgets displayed on the panel.
 */
class PanelWidget {
public:
	PanelWidget(const PWinObj* parent, const PanelTheme& theme,
		    const SizeReq& size_req);
	virtual ~PanelWidget(void);

	bool isDirty(void) const { return _dirty; }
	int getX(void) const { return _x; }
	int getRX(void) const { return _rx; }
	virtual void move(int x);

	uint getWidth(void) const { return _width; }
	void setWidth(uint width) {
		_width = width;
		_rx = _x + width;
	}

	const SizeReq& getSizeReq(void) const { return _size_req; }
	virtual uint getRequiredSize(void) const { return 0; }

	virtual void click(int, int) { }

	virtual void render(Render& render)
	{
		render.clear(_x, 0, _width, _theme.getHeight());
		_dirty = false;
	}

	/**
	 * Return true if win is one of PanelWidget windows (if any are used)
	 */
	virtual bool operator==(Window win) const { return false; }

	/**
	 * Handle X11 event, return true if the event was processed by
	 * this widget.
	 */
	virtual bool handleXEvent(XEvent* ev) { return false; }

protected:
	int renderText(Render &rend, PFont *font,
		       int x, const std::string& text, uint max_width);

protected:
	const PWinObj* _parent;
	const PanelTheme& _theme;
	bool _dirty;

private:
	int _x;
	int _rx;
	uint _width;
	SizeReq _size_req;
};

#endif // _PEKWM_PANEL_PANEL_WIDGET_HH_
