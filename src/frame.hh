//
// frame.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _FRAME_HH_
#define _FRAME_HH_

#include "screeninfo.hh"
#include "theme.hh"
#include "windowmanager.hh"

#include <list>
#include <vector>

class Client;

class Frame
{
public:
	Frame(WindowManager *w, Client *cl);
	~Frame();

	void loadTheme(void);

	void insertClient(Client *c, bool focus = true);
	void removeClient(Client *c);

	void activateClient(Client *c, bool focus = true);
	void activateNextClient(void);
	void activatePrevClient(void);
	void activateClientFromPos(int x);
	void activateClientFromNum(unsigned int n);

	void moveClientNext(void);
	void moveClientPrev(void);

	unsigned int getNumClients(void) const { return m_client_list.size(); }

	Client *getClientFromPos(unsigned int x);
	bool clientIsInFrame(Client *c);

	FrameButton *findButton(Window w);

	inline Client *getActiveClient(void) const { return m_client; }

	inline Window getFrameWindow(void) const { return m_frame; }
	inline Window getTitleWindow(void) const { return m_title; }

	inline int getX(void) const { return m_x; }
	inline int getY(void) const { return m_y; }
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }

	inline unsigned int getTitleHeight(void) const { return m_title_height; }

	// event handlers
	Action* handleButtonEvent(XButtonEvent *e);
	Action* handleMotionNotifyEvent(XMotionEvent *ev);

	void setFocus(bool focus);
	void setButtonFocus(bool f);
	void setBorder(void); // these makes sure the frame matches the clients
	void setTitlebar(void); // border / titlebar state
	inline void setHiddenState(bool h) { m_is_hidden = h; }

	void repaint(bool focus);

	void hide(void);
	void hideClient(Client *c);

	void unhide(bool restack = true);
	void unhideClient(Client *c);
	void iconifyAll(void);

	void showAllButtons(void);
	void hideAllButtons(void);

	void setWorkspace(unsigned int workspace);

	void move(int x, int y);
	void moveToCorner(Corner corner);
	void resizeHorizontal(int size_diff);
	void resizeVertical(int size_diff);
	void raise(bool restack = true);
	void lower(bool restack = true);
	void shade(void);
	void maximize(void);
	void maximizeVertical(void);
	void maximizeHorizontal(void);

	void doResize(bool left, bool x, bool top, bool y);

#ifdef SHAPE
	void setShape(void);
#endif

#ifdef MENUS
	void showWindowMenu(void);
#endif // MENUS

	void updateTitleHeight(void);
	void fixGeometryBasedOnStrut(Strut *strut);

	void updateFrameSize(void);
	void updateFrameSize(unsigned int w, unsigned int h);
	void updateFramePosition(void);
	void updateFramePosition(int nx, int ny);

	void updateFrameGeometry(void); // uppdates both size and position
	void updateFrameGeometry(int nx, int ny, unsigned int w, unsigned int h);

	void updateClientSize(void);
	void updateClientPosition(void);
	void updateClientGeometry(void); // uppdates both size and position

	bool allClientsAreOfSameClass(void);
	bool allClientsAreHidden(void);

	inline unsigned int borderTop(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_TOP]->getHeight()
			: theme->getWinUnfocusedBorder()[BORDER_TOP]->getHeight()) : 0);
	}

	inline unsigned int borderTopLeft(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_TOP_LEFT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_TOP_LEFT]->getWidth()) : 0);
	}

	inline unsigned int borderTopRight(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_TOP_RIGHT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_TOP_RIGHT]->getWidth()) : 0);
	}

	inline unsigned int borderBottom(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_BOTTOM]->getHeight()
			: theme->getWinUnfocusedBorder()[BORDER_BOTTOM]->getHeight()) : 0);
	}

	inline unsigned int borderBottomLeft(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_BOTTOM_LEFT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_BOTTOM_LEFT]->getWidth()) : 0);
	}

	inline unsigned int borderBottomRight(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_BOTTOM_RIGHT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_BOTTOM_RIGHT]->getWidth()) : 0);
	}

	inline unsigned int borderLeft(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_LEFT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_LEFT]->getWidth()) : 0);
	}
	inline unsigned int borderRight(void) const {
		return (m_client->hasBorder() ? (m_client->hasFocus()
			? theme->getWinFocusedBorder()[BORDER_RIGHT]->getWidth()
			: theme->getWinUnfocusedBorder()[BORDER_RIGHT]->getWidth()) : 0);
	}
private:
	void constructFrame(Client *cl);
	void createBorderWindows(void);

	void setBorderFocus(bool f);
	void rearangeBorderWindows(void);

	// button actions
	void loadButtons(void);
	void unloadButtons(void);
	void updateButtonPosition(void);

	void recalcResizeDrag(int nx, int ny, bool left, bool top);
	void checkEdgeSnap(void);

	// frame actions
	void doMove(XMotionEvent *ev);
	void drawWireframe(void);

	void clientGroupingDrag(XMotionEvent *ev);

	Action* findMouseButtonAction(unsigned int button, unsigned int mod,
																MouseButtonType type,
																std::list<MouseButtonAction> *actions);
	void initFrameState(void);
private:
	WindowManager *wm;
	ScreenInfo *scr;
	Theme *theme;

	std::vector<Client*> m_client_list;
	std::list<FrameButton*> m_button_list;

	Client *m_client;
	FrameButton *m_button;

	Window m_frame; // parent window which we reparent the client to
	Window m_title; // window which holds title

	Window m_border[BORDER_NO_POS];

	Pixmap m_title_pixmap;

	// frame position and size
	int m_x, m_y;
	unsigned int m_width, m_height;
	unsigned int m_title_height;

	// chache for not neading to realloc pixmap memory
	unsigned int m_pixmap_width, m_pixmap_height;

	// total width of all buttons
	unsigned int m_button_width_left, m_button_width_right;

	// The old position and dimensions of the frame, used
	// in the maximize function.
	int m_old_x, m_old_y;
	unsigned int m_old_width, m_old_height;

	// Used in client move
	int m_pointer_x, m_pointer_y;
	int m_old_cx, m_old_cy;

	int m_last_button_time[5]; // to hold double click times

	// switches
	bool m_is_hidden;
};

#endif // _FRAME_HH_
