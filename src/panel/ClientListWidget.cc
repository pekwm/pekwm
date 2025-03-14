//
// ClientListWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "ClientListWidget.hh"
#include "Debug.hh"
#include "String.hh"

ClientListWidget::ClientListWidget(const PanelWidgetData &data,
				   const PWinObj* parent,
				   const WidgetConfig& cfg,
				   const std::string& draw_separator)
	: PanelWidget(data, parent, cfg.getSizeReq(), cfg.getIf()),
	  _entry_width(0),
	  _draw_separator(pekwm::ascii_ncase_equal("separator",
						   draw_separator))
{
	// high priority, need to update client list _before_ rendering the
	// widget to avoid stale icon references
	pekwm::observerMapping()->addObserver(&_wm_state, this, 50);
}

ClientListWidget::~ClientListWidget(void)
{
	pekwm::observerMapping()->removeObserver(&_wm_state, this);
}

void
ClientListWidget::notify(Observable* observable, Observation* observation)
{
	if (dynamic_cast<WmState::ActiveWorkspace_Changed*>(observation)
	    || dynamic_cast<WmState::ActiveWindow_Changed*>(observation)
	    || dynamic_cast<WmState::ClientList_Changed*>(observation)
	    || dynamic_cast<WmState::ClientState_Changed*>(observation)) {
		_dirty = true;
		update();
	}
	PanelWidget::notify(observable, observation);
}

void
ClientListWidget::click(int button, int x, int)
{
	if (button != BUTTON1) {
		return;
	}
	Window window = findClientAt(x);
	if (window == None) {
		return;
	}

	Cardinal timestamp = 0;
	X11::getCardinal(window, NET_WM_USER_TIME, timestamp);
	P_TRACE("ClientListWidget activate " << window << " timestamp "
		<< timestamp);

	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = X11::getAtom(NET_ACTIVE_WINDOW);
	ev.xclient.window = window;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = timestamp;
	ev.xclient.data.l[2] = _wm_state.getActiveWindow();

	X11::sendEvent(X11::getRoot(), False,
		       SubstructureRedirectMask|SubstructureNotifyMask, &ev);
}

void
ClientListWidget::render(Render &rend, PSurface* surface)
{
	PanelWidget::render(rend, surface);

	uint height = _theme.getHeight() - 2;
	std::vector<Entry>::iterator it = _entries.begin();
	for (; it != _entries.end(); ++it) {
		PImage *icon = it->getIcon();
		int icon_width = icon ? height + 1 : 0;
		int x = getX() + it->getX() + icon_width;
		int entry_width = _entry_width - icon_width;
		if (_draw_separator && (it + 1) != _entries.end()) {
			PTexture* sep = _theme.getSep();
			entry_width -= sep->getWidth();
			sep->render(rend, x + entry_width, 0,
				    sep->getWidth(), sep->getHeight());
		}

		PFont *font = _theme.getFont(it->getState());
		int icon_x = renderText(rend, surface, font, x,
					it->getName(), entry_width);
		icon_x -= icon_width;

		if (icon) {
			if (icon->getHeight() > height) {
				icon->scale(height, height);
			}
			int icon_y = (height - icon->getHeight()) / 2;
			icon->draw(rend, surface, icon_x, icon_y);
		}
	}
}

Window
ClientListWidget::findClientAt(int x)
{
	std::vector<Entry>::iterator it = _entries.begin();
	for (; it != _entries.end(); ++it) {
		if (x >= it->getX()
		    && x <= (it->getX() + _entry_width)) {
			return it->getWindow();
		}
	}
	return None;
}

void
ClientListWidget::update(void)
{
	_entries.clear();
	createEntries();

	// no clients on active workspace, skip rendering and avoid
	// division by zero.
	if (_entries.empty()) {
		_entry_width = getWidth();
	} else {
		_entry_width = getWidth() / _entries.size();
	}

	int x = 0;
	std::vector<Entry>::iterator it = _entries.begin();
	for (; it != _entries.end(); ++it) {
		it->setX(x);
		x += _entry_width;
	}
}

void
ClientListWidget::createEntries()
{
	uint workspace = _wm_state.getActiveWorkspace();

	WmState::client_info_it it = _wm_state.clientsBegin();
	for (; it != _wm_state.clientsEnd(); ++it) {
		if (! (*it)->displayOn(workspace)) {
			continue;
		}
		ClientState state;
		if ((*it)->getWindow() == _wm_state.getActiveWindow()) {
			state = CLIENT_STATE_FOCUSED;
		} else if ((*it)->hidden) {
			state = CLIENT_STATE_ICONIFIED;
		} else {
			state = CLIENT_STATE_UNFOCUSED;
		}
		_entries.push_back(Entry((*it)->getName(),
					 state, 0,
					 (*it)->getWindow(),
					 (*it)->getIcon()));
	}
}
