//
// WindowManager.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// windowmanager.hh for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _WINDOWMANAGER_HH_
#define _WINDOWMANAGER_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "Atoms.hh"
#include "ScreenInfo.hh"

#include <list>
#include <map>

class ActionHandler;
class AutoProperties;
class Config;
class Theme;
class Workspaces;

class Frame;
class Client;
class ClassHint;

class AutoProperty; // for findGroup

#ifdef KEYS
class KeyGrabber;
#endif // KEYS
#ifdef HARBOUR
class Harbour;
#endif // HARBOUR

#ifdef MENUS
class BaseMenu;
#endif // MENUS

void sigHandler(int signal);
int handleXError(Display *dpy, XErrorEvent *e);

class WindowManager
{
public:
	WindowManager(std::string command_line);
	~WindowManager();

	void reload(void);
	void restart(std::string command = "");

	void quitNicely(void); // Cleans up and exits the window manager.

	// get "base" classes
	inline ScreenInfo *getScreen(void) const { return _screen; }
	inline Config *getConfig(void) const { return _config; }
	inline Theme *getTheme(void) const { return _theme; }
	inline ActionHandler *getActionHandler(void)
		const { return _action_handler; }
	inline AutoProperties* getAutoProperties(void) const {
		return _autoproperties; }
	inline Workspaces *getWorkspaces(void) const { return _workspaces; }
#ifdef KEYS
	inline KeyGrabber *getKeyGrabber(void) const { return _keygrabber; }
#endif // KEYS
#ifdef HARBOUR
	inline Harbour *getHarbour(void) const { return _harbour; }
#endif //HARBOUR

	inline bool isStartup(void) const { return _startup; }

	inline const std::list<Frame*> &getFrameList(void) const {
		return _frame_list; }
	inline const std::list<Client*> &getClientList(void) const {
		return _client_list; }

	inline Client *getFocusedClient(void) const { return _focused_client; }

	// Menus
#ifdef MENUS
	inline BaseMenu* getMenu(MenuType type) {
		std::map<MenuType, BaseMenu*>::iterator it = _menus.find(type);
		if (it != _menus.end())
			return it->second;
		return NULL;
	}

#endif // MENUS

	// Adds
	inline void addToClientList(Client *c) { if (c) _client_list.push_back(c); }
	inline void addToFrameList(Frame *f) { if (f) _frame_list.push_back(f); }

	// Removes
	void removeFromClientList(Client *client);
	void removeFromFrameList(Frame *frame);

	Client *findClient(Window win);
	Client *findClientFromWindow(Window win);
	Frame *findFrameFromWindow(Window win);

	// focus / stacking
	void focusNextPrevFrame(unsigned int button, unsigned int raise,
													bool next);
													

	// If a window unmaps and has transients lets unmap them too!
	// true = hide | false = unhide
	void findTransientsToMapOrUnmap(Window win, bool hide);

	void setWorkspace(unsigned int workspace, bool focus);
	bool warpWorkspace(int diff, bool wrap, bool focus, int x, int y);
	void setNumWorkspaces(unsigned int num);

	Client* findClient(const ClassHint* class_hint);
	Frame* findGroup(const ClassHint* class_hint, AutoProperty* property);
	Frame* findTaggedFrame(bool &activate);
	Frame* findFrameFromId(unsigned int id);

	unsigned int findUniqueFrameId(void);
	void setupFrameIds(void);

	inline Frame* getTaggedFrame(void) { return _tagged_frame; }
	void setTaggedFrame(Frame *frame, bool activate);

	void attachMarked(Frame *frame);
	void attachInNextPrevFrame(Client* client, bool frame, bool next);

	Frame* getNextFrame(Frame* frame, bool mapped, unsigned int mask = 0);
	Frame* getPrevFrame(Frame* frame, bool mapped, unsigned int mask = 0);

	void findFrameAndFocus(Window win);

#ifdef MENUS
	void hideAllMenus(void);
#endif // MENUS

	// this is for showing status on the screen while moving / resizing
	void showStatusWindow(void) const;
	void hideStatusWindow(void) const;
	void drawInStatusWindow(const std::string &text, int x = -1, int y = -1);

	// click time set and get
	inline Time getLastFrameClick(unsigned int button) {
		if (button <= NUM_BUTTONS)
			return _last_frame_click[button];
		return 0;
	}
	inline void setLastFrameClick(unsigned int button, Time time) {
		if (button < NUM_BUTTONS)
			_last_frame_click[button] = time;
	}

	// Methods for the various hints
	inline PekwmAtoms *getPekwmAtoms(void) { return _pekwm_atoms; }
	inline IcccmAtoms *getIcccmAtoms(void) { return _icccm_atoms; }
	inline EwmhAtoms *getEwmhAtoms(void) { return _ewmh_atoms; }

	inline Atom getMwmHintsAtom(void) { return _atom_mwm_hints; }

	// Hint methods
	long getAtomLongValue(Window win, int atom);
	void setAtomLongValue(Window win, int atom, long value);

	// Extended Window Manager hints function prototypes
	void setEwmhHintString(Window w, int a, char* value);
	bool getEwmhHintString(Window w, int a, std::string &name);
	void* getEwmhPropertyData(Window win, Atom prop,
																	 Atom type, int *items);
	bool getEwmhStates(Window win, NetWMStates &win_state);
	void setEwmhSupported(void);
	void setEwmhDesktopGeometry(void);
	void setEwmhDesktopViewport(void);
	void setEwmhActiveWindow(Window w);
	void setEwmhWorkArea(void);

private:
	void setupDisplay(void);
	void scanWindows(void);
	void execStartFile(void);

	void cleanup(void);

	// screen edge related
	void screenEdgeCreate(void);
	void screenEdgeDestroy(void);
	void screenEdgeResize(void);
	void screenEdgeMap(void);
	void screenEdgeUnmap(void);
	ScreenEdgeType screenEdgeFind(Window window);

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
	void handleFocusOutEvent(XFocusChangeEvent *ev);

	// helpers for the event handlers
	ActionEvent* findMouseButtonAction(XButtonEvent *ev, MouseButtonType type);

	// private methods for the hints
	void initHints(void);

private:
	ScreenInfo *_screen;
#ifdef KEYS
	KeyGrabber *_keygrabber;
#endif // KEYS
	Config *_config;
	Theme *_theme;
	ActionHandler *_action_handler;
	AutoProperties *_autoproperties;
	Workspaces *_workspaces;
#ifdef HARBOUR
	Harbour *_harbour;
#endif // HARBOUR

#ifdef MENUS
	BaseMenu *_focused_menu;
	std::map<MenuType, BaseMenu*> _menus;
#endif // MENUS

	const char *_wm_name;
	std::string _command_line;
	bool _startup;

	std::list<Frame*> _frame_list;
	std::list<Client*> _client_list;
	std::list<unsigned int> _frameid_list; // stores removed frame id's

	Client *_focused_client;

	Frame *_tagged_frame;
	bool _tag_activate;

	std::list<ScreenInfo::ScreenEdge*> _screen_edge_list;
	Window _status_window; // TO-DO: Convert to WindowObject

	// Hold double click times
	Time _last_root_click[NUM_BUTTONS];
	Time _last_frame_click[NUM_BUTTONS];

	// Atoms and hints follow under here
	PekwmAtoms *_pekwm_atoms;
	IcccmAtoms *_icccm_atoms;
	EwmhAtoms *_ewmh_atoms;

	// Atom for motif hints
	Atom _atom_mwm_hints;

	// Windows for the different hints
	Window _extended_hints_win;
};

#endif // _WINDOWMANAGER_HH_
