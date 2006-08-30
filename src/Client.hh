//
// Client.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// client.hh for aewm++
// Copyright (C) 2002 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _CLIENT_HH_
#define _CLIENT_HH_

#include "pekwm.hh"

class ScreenInfo;
class Strut;
class WindowObject;
class ClassHint;
class AutoProperty;
class Frame;
class WindowManager;

#include <string>

extern "C" {
#include <X11/Xutil.h>
}

class Client : public WindowObject
{
	// TO-DO: This relationship should end as soon as possible, but I need to
	// figure out a good way of sharing. :)
	friend class Frame;

public: // Public Member Functions
	struct MwmHints {
		unsigned long flags;
		unsigned long functions;
		unsigned long decorations;
	};
	enum MwmFunc {
		MWM_FUNC_ALL = (1l << 0),
		MWM_FUNC_RESIZE = (1l << 1),
		MWM_FUNC_MOVE = (1l << 2),
		MWM_FUNC_ICONIFY = (1l << 3),
		MWM_FUNC_MAXIMIZE = (1l << 4),
		MWM_FUNC_CLOSE = (1l << 5)
	};
	enum MwmDecor {
		MWM_DECOR_ALL = (1l << 0),
		MWM_DECOR_BORDER = (1l << 1),
		MWM_DECOR_HANDLE = (1l << 2),
		MWM_DECOR_TITLE = (1l << 3),
		MWM_DECOR_MENU = (1l << 4),
		MWM_DECOR_ICONIFY = (1l << 5),
		MWM_DECOR_MAXIMIZE = (1l << 6)
	};

	Client(WindowManager *w, Window new_client);
	~Client();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	virtual void iconify(void);

	virtual void move(int x, int y);
	virtual void resize(unsigned int width, unsigned int height);

	virtual void setWorkspace(unsigned int workspace);
	// END - WindowObject interface.

	bool validate(void);

	inline std::string* getName(void) { return &_name; }
	inline std::string* getIconName(void) { return &_icon_name; }

	inline const ClassHint* getClassHint(void) const { return _class_hint; }

	inline Window getTransientWindow(void) const { return _transient; }
	inline Frame* getFrame(void) const { return _frame; }
	inline XSizeHints* getXSizeHints(void) const { return _size; }

	bool getGeometry(Geometry &gm);
	bool setPUPosition(Geometry &gm);

	inline bool hasTitlebar(void) const { return _titlebar; }
	inline bool hasBorder(void) const { return _border; }
#ifdef SHAPE
	inline bool isShaped(void) const { return _shaped; }
#endif // SHAPE
	inline bool hasStrut(void) const { return (_strut); }

	// State accessors
	inline bool isMaximizedVert(void) const { return _state.maximized_vert; }
	inline bool isMaximizedHorz(void) const { return _state.maximized_horz; }
	inline bool isShaded(void) const { return _state.shaded; }
	inline bool skipTaskbar(void) const { return _state.skip_taskbar; }
	inline bool skipPager(void) const { return _state.skip_pager; }
	inline bool isPlaced(void) const { return _state.placed; }
	inline unsigned int getSkip(void) const { return _state.skip; }

	inline bool allowMove(void) const { return _actions.move; }
	inline bool allowResize(void) const { return _actions.resize; }
	inline bool allowMinimize(void) const { return _actions.minimize; }
	inline bool allowShade(void) const { return _actions.shade; }
	inline bool allowStick(void) const { return _actions.stick; }
	inline bool allowMaximizeHorz(void) const { return _actions.maximize_horz; }
	inline bool allowMaximizeVert(void) const { return _actions.maximize_vert; }
	inline bool allowFullscreen(void) const { return _actions.fullscreen; }
	inline bool allowChangeDesktop(void) const { return _actions.change_desktop; }
	inline bool allowClose(void) const { return _actions.close; }

	inline bool isAlive(void) const { return _alive; }
	inline bool isMarked(void) const { return _marked; }

	// We have this public so that we can reload button actions.
	void grabButtons(void);

	// toggles
	void stick(void);
	void alwaysOnTop(bool top);
	void alwaysBelow(bool bottom);
	inline void mark(void) { _marked = !_marked; }

	inline void setBorder(bool border) { _border = border; }
	inline void setTitlebar(bool titlebar) { _titlebar = titlebar; }
#ifdef SHAPE
	inline void setShaped(bool s) { _shaped = s; }
#endif // SHAPE

	void close(void);
	void kill(void);

	// Event handlers below - Used by WindowManager
	void handleMapRequest(XMapRequestEvent *);
	void handleUnmapEvent(XUnmapEvent *);
	void handleDestroyEvent(XDestroyWindowEvent *);
	void handleColormapChange(XColormapEvent *);

	void sendConfigureRequest(void);

	bool getIncSize(unsigned int *r_w, unsigned int *r_h,
		unsigned int w, unsigned int h);

	void updateWmStates(void);

	void getWMNormalHints(void);
	void getTransientForHint(void);
	void getStrutHint(void);

	void filteredReparent(Window parent, int x, int y);

private: // Private Member Functions
	void getXClientName(void);
	void getXIconName(void); // Not currently using Icon name for any purpose.

	void setWmState(unsigned long state);
	long getWmState(void);

	int sendXMessage(Window, Atom, long, long);

	MwmHints* getMwmHints(Window w);

	// these are used by frame
	inline void setFrame(Frame *f) { _frame = f; }
	inline void setMaximizedVert(bool m) { _state.maximized_vert = m; }
	inline void setMaximizedHorz(bool m) { _state.maximized_horz = m; }
	inline void setShade(bool s) { _state.shaded = s; }

	// Grabs button with Caps,Num and so on
	void grabButton(int button, int mod, int mask, Window win, Cursor curs);

	void readEwmhHints(void);
	void readMwmHints(void);
	AutoProperty* readAutoprops(unsigned int type = 0);

private: // Private Member Variables
	WindowManager *_wm;

	XSizeHints *_size;
	Colormap _cmap;

	Window _transient; // window id for which this client is transient for

	Frame *_frame;
	Strut *_strut;

	std::string _name, _icon_name; // Name used to display in titlebar

	ClassHint *_class_hint;

	bool _alive, _marked;
	bool _titlebar, _border;
	bool _extended_net_name;
#ifdef SHAPE
	bool _shaped;
#endif // SHAPE

	class State {
	public:
		State() : maximized_vert(false), maximized_horz(false),
							shaded(false), skip_taskbar(false), skip_pager(false),
							placed(false), skip(0) { }

		// not supported	bool modal;
		bool maximized_vert;
		bool maximized_horz;
		bool shaded;
		bool skip_taskbar;
		bool skip_pager;
		// not supported	bool fullscreen;
		// not supported	bool above;
		// not supported	bool below;

		bool placed; // pekwm
		unsigned int skip; // pekwm
	} _state;

	class Actions {
	public:
		Actions() : move(true), resize(true), minimize(true),
								shade(true), stick(true), maximize_horz(true),
								maximize_vert(true), fullscreen(true),
								change_desktop(true), close(true) { }

		bool move;
		bool resize;
		bool minimize; // iconify
		bool shade;
		bool stick;
		bool maximize_horz;
		bool maximize_vert;
		bool fullscreen;
		bool change_desktop; // workspace
		bool close;
	} _actions;

	// MOTIF hints
	static const long MWM_HINTS_FUNCTIONS = (1l << 0);
	static const long MWM_HINTS_DECORATIONS = (1l << 1);
	static const unsigned int MWM_HINT_ELEMENTS = 3;
};

#endif // _CLIENT_HH_
