//
// windowmanager.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// windowmanager.hh for aewm++
// Copyright (C) 2000 Frank Hale
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

#ifndef _WINDOWMANAGER_HH_
#define _WINDOWMANAGER_HH_

#include "pekwm.hh"
#include "screeninfo.hh"
#include "atoms.hh"
#include "actionhandler.hh"
#include "autoprops.hh"
#include "config.hh"
#include "theme.hh"
#ifdef KEYS
#include "keys.hh"
#endif // KEYS
#ifdef HARBOUR
#include "harbour.hh"
#endif // HARBOUR

#include "workspaces.hh"
#include "client.hh"

class Frame; // forward declaration to break cyclics

#ifdef MENUS
#include "windowmenu.hh"
#include "rootmenu.hh"
#include "iconmenu.hh"
#endif // MENUS

void sigHandler(int signal);
int handleXError(Display *dpy, XErrorEvent *e);

class WindowManager
{
public:
	// If adding, make sure it "syncs" with the theme border enum
	// BAD HABIT
	enum {
		TOP_LEFT_CURSOR, TOP_CURSOR, TOP_RIGHT_CURSOR,
		LEFT_CURSOR, RIGHT_CURSOR,
		BOTTOM_LEFT_CURSOR, BOTTOM_CURSOR, BOTTOM_RIGHT_CURSOR,
		ARROW_CURSOR, MOVE_CURSOR, RESIZE_CURSOR,
		NUM_CURSORS
	};

	WindowManager(std::string command_line);
	~WindowManager();

	void reload(void);
	void restart(std::string command = "");

	void quitNicely(void); // Cleans up and exits the window manager.

	// get "base" classes
	inline ScreenInfo *getScreen(void) const { return m_screen; }
	inline Config *getConfig(void) const { return m_config; }
	inline Theme *getTheme(void) const { return m_theme; }
	inline ActionHandler *getActionHandler(void)
		const { return m_action_handler; }
	inline AutoProps *getAutoProps(void) const { return m_autoprops; }
	inline Workspaces *getWorkspaces(void) const { return m_workspaces; }
#ifdef KEYS
	inline Keys *getKeys(void) const { return m_keys; }
#endif // KEYS
#ifdef HARBOUR
	inline Harbour *getHarbour(void) const { return m_harbour; }
#endif //HARBOUR

	inline Client *getFocusedClient(void) const { return m_focused_client; }
	inline std::vector<Client*> *getClientList(void) { return &m_client_list; }
	inline unsigned int getActiveWorkspace(void) const {
		return m_active_workspace; }

	inline std::list<Frame*> *getFrameList(void) { return &m_frame_list; }

	// Cursors
	inline Cursor* getCursors(void) { return m_cursors; }

	// Menus
#ifdef MENUS
	inline RootMenu *getRootMenu(void) const { return m_rootmenu; }
	inline WindowMenu *getWindowMenu(void) const { return m_windowmenu; }
	inline IconMenu *getIconMenu(void) const { return m_iconmenu; }
#endif // MENUS

#ifdef SHAPE
	inline bool hasShape(void) const { return m_has_shape; }
	inline int getShapeEvent(void) const { return m_shape_event; }
#endif

	inline Strut *getMasterStrut(void) const { return m_master_strut; }

	inline char *getWindowManagerName(void) const { return m_wm_name; }
	void getMousePosition(int *x, int *y);

	// End of gets

	// Adds
	inline void addToClientList(Client *c) { if (c) m_client_list.push_back(c); }
	inline void addToFrameList(Frame *f) { if (f) m_frame_list.push_back(f); }

	void addStrut(Strut *strut);

	// Removes
	void removeFromClientList(Client *c);
	inline void removeFromFrameList(Frame *f) {
		if (m_frame_list.size()) m_frame_list.remove(f);
	}
	void removeStrut(Strut *rem_strut);

	Client *findClient(Window w);
	Client *findClientFromWindow(Window w);

	// Iconmenu
#ifdef MENUS
	void updateIconMenu(void);
	void addClientToIconMenu(Client *c);
	void removeClientFromIconMenu(Client *c);
#endif // MENUS

	// focus / stacking
	void focusNextFrame(void);

	// If a window unmaps and has transients lets unmap them too!
	// true = hide | false = unhide
	void findTransientsToMapOrUnmap(Window win, bool hide);

	void setWorkspace(unsigned int workspace, bool focus);
	void setNumWorkspaces(unsigned int num);

	Client *findClient(const AutoProps::ClassHint &class_hint);
	Frame *findGroup(const AutoProps::ClassHint &class_hint,
									 unsigned int desktop, unsigned int max);

	void findClientAndFocus(Window win);

	// this is for showing status on the screen while moving / resizing
	inline void showStatusWindow(void) { XMapRaised(dpy, m_status_window); }
	inline void hideStatusWindow(void) { XUnmapWindow(dpy, m_status_window); }
	void drawInStatusWindow(const std::string &text, int x = -1, int y = -1);

	// Methods for the various hints

	inline IcccmAtoms *getIcccmAtoms(void) { return m_icccm_atoms; }
	inline EwmhAtoms *getEwmhAtoms(void) { return m_ewmh_atoms; }

	inline Atom getMwmHintsAtom(void) { return m_atom_mwm_hints; }

	long getHint(Window w, int a);

	// Extended Window Manager hints function prototypes
	int sendExtendedHintMessage(Window w, Atom a, long mask, long int data[]);
	void setExtendedWMHint(Window w, int a, long value);
	void setExtendedWMHintString(Window w, int a, char* value);
	bool getExtendedWMHintString(Window w, int a, std::string &name);
	void* getExtendedNetPropertyData(Window win, Atom prop,
																	 Atom type, int *items);
	bool getExtendedNetWMStates(Window win, NetWMStates &win_state);
	void setExtendedNetWMState(Window win,
														 bool modal, bool sticky,
														 bool maxvert, bool max_horz,
														 bool shaded, bool skip_taskbar, bool skip_pager,
														 bool hidden, bool fullscreen);
	void setExtendedNetSupported(void);
	void setExtendedNetDesktopGeometry(void);
	void setExtendedNetDesktopViewport(void);
	void setExtendedNetActiveWindow(Window w);
	void setExtendedNetWorkArea(void);

	int findDesktopHint(Window win);
	long getDesktopHint(Window win, int a);
private:
	void setupDisplay(void);
	void scanWindows(void);
	void execStartScript(void);

	void cleanup(void);

	// event handling
	void doEventLoop(void);

	void handleMapRequestEvent(XMapRequestEvent *ev);
	void handleUnmapEvent(XUnmapEvent *ev);
	void handleDestroyWindowEvent(XDestroyWindowEvent *ev);

	void handleConfigureRequestEvent(XConfigureRequestEvent *ev);
	void handleClientMessageEvent(XClientMessageEvent *ev);

	void handleColormapEvent(XColormapEvent *ev);
	void handlePropertyEvent(XPropertyEvent *ev);
	void handleExposeEvent(XExposeEvent *ev);

	void handleKeyEvent(XKeyEvent *ev);

	void handleButtonPressEvent(XButtonEvent *ev);
	void handleButtonReleaseEvent(XButtonEvent *ev);

	void handleMotionEvent(XMotionEvent *ev);

	void handleEnterNotify(XCrossingEvent *ev);
	void handleLeaveNotify(XCrossingEvent *ev);
	void handleFocusInEvent(XFocusChangeEvent *ev);

	// helpers for the event handlers
	Action* findMouseButtonAction(unsigned int button);

	// private methods for the hints
	void initHints(void);

private:
	ScreenInfo *m_screen;
#ifdef KEYS
	Keys *m_keys;
#endif // KEYS
	Config *m_config;
	Theme *m_theme;
	ActionHandler *m_action_handler;
	AutoProps *m_autoprops;
	Workspaces *m_workspaces;
#ifdef HARBOUR
	Harbour *m_harbour;
#endif // HARBOUR

	Display *dpy;
	Window root;
	Cursor m_cursors[NUM_CURSORS];

#ifdef MENUS
	WindowMenu *m_windowmenu;
	IconMenu *m_iconmenu;
	RootMenu *m_rootmenu;
#endif // MENUS

	std::vector<Client*> m_client_list;
	std::list<Frame*> m_frame_list;
	Client *m_focused_client;

	char *m_wm_name;
	unsigned int m_active_workspace;

	std::string m_command_line;

	Window m_status_window;

#ifdef SHAPE
	bool m_has_shape;
	int m_shape_event;
#endif

	// Maintains the master struts
	// Apps will add or subtract from the master strut.
	Strut *m_master_strut;
	std::vector<Strut*> m_strut_list;

	// Atoms and hints follow under here
	IcccmAtoms *m_icccm_atoms;
	EwmhAtoms *m_ewmh_atoms;

	// Atom for motif hints
	Atom m_atom_mwm_hints;

	Atom m_net_wm_states[NET_WM_MAX_STATES];

	// Windows for the different hints
	Window m_extended_hints_win;
};

#endif // _WINDOWMANAGER_HH_
