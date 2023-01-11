//
// ClientListWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_CLIENT_LIST_WIDGET_HH_
#define _PEKWM_PANEL_CLIENT_LIST_WIDGET_HH_

#include <string>
#include <vector>

#include "pekwm_panel.hh"
#include "Observable.hh"
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "PImage.hh"
#include "WmState.hh"

/**
 * List of Frames/Clients on the current workspace.
 */
class ClientListWidget : public PanelWidget,
			 public Observer {
public:
	class Entry {
	public:
		Entry(const std::string& name, ClientState state, int x,
		      Window window, PImage *icon)
			: _name(name),
			  _state(state),
			  _x(x),
			  _window(window),
			  _icon(icon)
		{
		}

		const std::string &getName(void) const { return _name; }
		ClientState getState(void) const { return _state; }
		int getX(void) const { return _x; }
		void setX(int x) { _x = x; }
		Window getWindow(void) const { return _window; }
		PImage *getIcon(void) const { return _icon; }

	private:
		std::string _name;
		ClientState _state;
		int _x;
		Window _window;
		PImage *_icon;
	};

	ClientListWidget(const PWinObj* parent,
			 const PanelTheme& theme,
			 const SizeReq& size_req,
			 WmState& wm_state);
	virtual ~ClientListWidget(void);

	virtual void notify(Observable*, Observation*);
	virtual void click(int x, int);
	virtual void render(Render &rend);

private:
	Window findClientAt(int x);
	void update(void);
	void createEntries(void);

private:
	WmState& _wm_state;
	int _entry_width;
	std::vector<Entry> _entries;
};

#endif // _PEKWM_PANEL_CLIENT_LIST_WIDGET_HH_
