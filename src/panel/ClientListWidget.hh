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
#include "WmState.hh"

#include "../tk/PImage.hh"

/**
 * List of Frames/Clients on the current workspace.
 *
 * ClientList [separator]
 */
class ClientListWidget : public PanelWidget {
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

	ClientListWidget(const PanelWidgetData &data, const PWinObj* parent,
			 const WidgetConfig& cfg,
			 const std::string& draw_separator);
	virtual ~ClientListWidget(void);

	virtual const char *getName() const { return "ClientList"; }

	virtual void notify(Observable*, Observation*);
	virtual void click(int button, int x, int);
	virtual void render(Render &rend, PSurface* surface);

private:
	void initOptions(const std::string& options);

	Window findClientAt(int x);
	void update(void);
	void createEntries(void);

	int _entry_width;
	std::vector<Entry> _entries;
	bool _draw_icon;
	bool _draw_separator;
};

#endif // _PEKWM_PANEL_CLIENT_LIST_WIDGET_HH_
