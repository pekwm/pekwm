//
// PanelWidget.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_PANEL_WIDGET_HH_
#define _PEKWM_PANEL_PANEL_WIDGET_HH_

#include <string>

#include "pekwm_panel.hh"
#include "PanelAction.hh"
#include "PanelTheme.hh"
#include "VarData.hh"
#include "WmState.hh"

#include "../tk/PWinObj.hh"

class PanelWidgetData {
public:
	PanelWidgetData(Os *os_, const PanelTheme &theme_,
			VarData &var_data_, WmState &wm_state_)
		: os(os_),
		  theme(theme_),
		  var_data(var_data_),
		  wm_state(wm_state_)
	{
	}

	Os *os;
	const PanelTheme &theme;
	VarData &var_data;
	WmState &wm_state;
};

/**
 * Base class for all widgets displayed on the panel.
 */
class PanelWidget {
public:
	PanelWidget(const PanelWidgetData &data, const PWinObj* parent,
		    const SizeReq& size_req);
	virtual ~PanelWidget(void);

	virtual const char *getName() const = 0;

	bool isDirty(void) const { return _dirty; }
	bool isVisible(void) const { return _width != 0; }

	int getX(void) const { return _x; }
	int getRX(void) const { return _rx; }

	void setButtonAction(int button, const PanelAction &action);
	virtual void move(int x);

	uint getWidth(void) const { return _width; }
	void setWidth(uint width) {
		_width = width;
		_rx = _x + width;
	}

	const SizeReq& getSizeReq(void) const { return _size_req; }
	virtual void scaleChanged() { };
	virtual uint getRequiredSize(void) const { return 0; }

	virtual void click(int button, int x, int y);
	virtual void render(Render& render, PSurface*)
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
	int renderText(Render &rend, PSurface *surface, PFont *font,
		       int x, const std::string& text, uint max_width);

	void runAction(const PanelAction &action);
	void runActionExec(const std::string &param,
			   const std::string &command);
	void runActionPekwm(const std::string &param,
			    const std::string &command);

	Os *_os;
	const PanelTheme &_theme;
	VarData &_var_data;
	WmState &_wm_state;
	const PWinObj* _parent;

	bool _dirty;
	std::string _condition;
	std::map<int, PanelAction> _button_actions;

private:
	int _x;
	int _rx;
	uint _width;
	SizeReq _size_req;
};

#endif // _PEKWM_PANEL_PANEL_WIDGET_HH_
