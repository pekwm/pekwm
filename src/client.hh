//
// client.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// client.hh for aewm++
// Copyright (C) 2002 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
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
 
#ifndef _CLIENT_HH_
#define _CLIENT_HH_

#include "pekwm.hh"
#include "autoprops.hh"

#include <string>

#include <X11/Xutil.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif

class Frame;
class WindowManager;

class Client
{
	// TO-DO: This relationship should end as soon as possible, but I need to
	// figure out a good way of sharing. :)
	friend class Frame;
	
public: // Public Member Functions
	Client(WindowManager *w, Window new_client);
	~Client();

	inline char *getClientName(void) const { return m_name; }
	inline char *getClientIconName(void) const { return m_icon_name; }

	inline const AutoProps::ClassHint &getClassHint(void) const {
		return m_class_hint; }
	
	inline Window getWindow(void) const { return m_window; }
	inline Window getTransientWindow(void) const { return m_trans; } 
	inline Frame *getFrame(void) const { return m_frame; }
	inline XSizeHints *getXSizeHints(void) const { return m_size; }

	inline int getX(void) const { return m_x; }
	inline int getY(void) const { return m_y; }
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }
	unsigned int calcTitleHeight(void);

	inline int onWorkspace(void) const { return m_on_workspace; }

	inline bool hasFocus(void) const { return m_has_focus; }
	inline bool hasTitlebar(void) const { return m_has_titlebar; }
	inline bool hasBorder(void) const { return m_has_border; }
#ifdef SHAPE
	inline bool hasBeenShaped(void) const { return m_has_been_shaped; }
#endif // SHAPE
	inline bool hasStrut(void) const { return m_has_strut; }
	
	inline bool isSticky(void) const { return m_is_sticky; }
	inline bool isShaded(void) const { return m_is_shaded; }
	inline bool isIconified(void) const { return m_is_iconified; }
	inline bool isMaximized(void) const { return m_is_maximized; }
	inline bool isMaximizedVertical(void) const
		{ return m_is_maximized_vertical; }
	inline bool isMaximizedHorizontal(void) const
		{ return m_is_maximized_horizontal; }
	inline bool isHidden(void) const { return m_is_hidden; }
	inline bool isAlive(void) const { return m_is_alive; }
	inline unsigned int getLayer(void) const { return m_win_layer; }

	inline bool skipTaskbar(void) const { return m_skip_taskbar; }

	void hide(bool notify_frame = true);
	void unhide(bool notify_frame = true);

	// toggles
	void stick(void);
	void alwaysOnTop(void);
	void alwaysBelow(void);

	void iconify(bool notify_frame = true);
	void kill(void);

	void setWorkspace(unsigned int workspace);

	// Event handlers below - Used by WindowManager
	void handleConfigureRequest(XConfigureRequestEvent *);
	void handleMapRequest(XMapRequestEvent *);
	void handleUnmapEvent(XUnmapEvent *);
	void handleDestroyEvent(XDestroyWindowEvent *);
	void handleClientMessage(XClientMessageEvent *);
	void handlePropertyChange(XPropertyEvent *);
	void handleColormapChange(XColormapEvent *);
	void handleEnterEvent(XCrossingEvent *);
//	void handle_leave_event(XCrossingEvent *e);
	void handleFocusInEvent(XFocusChangeEvent *);
	void handleFocusOutEvent(XFocusChangeEvent *);
#ifdef SHAPE
	void handleShapeChange(XShapeEvent *);
#endif

	void setFocus(void); 

	void resize(unsigned int w, unsigned int h);
	void move(int x, int y); // just uppdates the client variables

	bool getIncSize(unsigned int *r_w, unsigned int *r_h,
		unsigned int w, unsigned int h);

	void loadAutoProps(int type);

	void updateWmStates(void);

	void filteredUnmap(void);
	void filteredReparent(Window parent, int x, int y);

private: // Private Member Functions
	void constructClient(void);
	bool placeSmart(void);
	bool placeCenteredUnderMouse(void);
	bool placeTopLeftUnderMouse(void);

	void placeInsideScreen(void); // helper for mouse placements
	void getFrameSize(unsigned int &width, unsigned int &height); // helper

	void getXClientName(void);
	void getXIconName(void); // Not currently using Icon name for any purpose.

	bool setPUPosition(void); // sets the client position to P/U position.

	void setWmState(unsigned long state);
	long getWmState(void);
	void gravitate(int);
	void sendConfigureRequest(void);
	int sendXMessage(Window, Atom, long, long);

	MwmHints* getMwmHints(Window w);

	// these are used by frame
	inline void setFrame(Frame *f) { m_frame = f; }
#ifdef SHAPE
	inline void setShaped(bool s) { m_has_been_shaped = s; }
#endif // SHAPE
	inline void setShade(bool s) { m_is_shaded = s; }
	inline void setMaximized(bool m) { m_is_maximized = m; }
	inline void setVertMaximized(bool m) { m_is_maximized_vertical = m; }
	inline void setHorizMaximized(bool m) { m_is_maximized_horizontal = m; }

	// Grabs button with Caps,Num and so on
	void grabButton(int button, int mod, int mask, Window win, Cursor curs);

	void initHintProperties(void);


private: // Private Member Variables 
	WindowManager *wm;
	Display *dpy;

	XSizeHints *m_size;
	Colormap m_cmap;

	Window m_window; // actual client window
	Window m_trans; // window id for which this client is transient for

	Frame *m_frame;

	char *m_name; // Name used to display in titlebar
	char *m_icon_name;

	AutoProps::ClassHint m_class_hint;

	// The position and dimensions of the client window
	int m_x, m_y;
	unsigned int m_width, m_height;

	bool m_has_focus;
	bool m_has_titlebar;
	bool m_has_border;

	bool m_is_shaded;
	bool m_is_iconified;
	bool m_is_maximized;
	bool m_is_maximized_vertical;
	bool m_is_maximized_horizontal;
	bool m_is_sticky;
	bool m_is_hidden;

	unsigned int m_win_layer;

#ifdef SHAPE
	bool m_has_been_shaped;
#endif // SHAPE

	Strut *m_client_strut;
	bool m_has_strut;
	bool m_has_extended_net_name;
	bool m_skip_taskbar;
	bool m_skip_pager;

	int m_on_workspace; // int becase of error message

	bool m_is_alive;
};

#endif // _CLIENT_HH_
