//
// Client.hh for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
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

class PScreen;
class Strut;
class PWinObj;
class ClassHint;
class AutoProperty;
class Frame;
class WindowManager;
class PDecor::TitleItem;

#include <string>

extern "C" {
#include <X11/Xutil.h>
}

class Client : public PWinObj
{
	// FIXME: This relationship should end as soon as possible, but I need to
	// figure out a good way of sharing. :)
	friend class Frame;

public: // Public Member Functions
	struct MwmHints {
		ulong flags;
		ulong functions;
		ulong decorations;
	};
	enum {
		MWM_HINTS_FUNCTIONS = (1L << 0),
		MWM_HINTS_DECORATIONS = (1L << 1),
		MWM_HINTS_NUM = 3
	};
	enum {
		MWM_FUNC_ALL = (1L << 0),
		MWM_FUNC_RESIZE = (1L << 1),
		MWM_FUNC_MOVE = (1L << 2),
		MWM_FUNC_ICONIFY = (1L << 3),
		MWM_FUNC_MAXIMIZE = (1L << 4),
		MWM_FUNC_CLOSE = (1L << 5)
	};
	enum {
		MWM_DECOR_ALL = (1L << 0),
		MWM_DECOR_BORDER = (1L << 1),
		MWM_DECOR_HANDLE = (1L << 2),
		MWM_DECOR_TITLE = (1L << 3),
		MWM_DECOR_MENU = (1L << 4),
		MWM_DECOR_ICONIFY = (1L << 5),
		MWM_DECOR_MAXIMIZE = (1L << 6)
	};

	Client(WindowManager *w, Window new_client, bool is_new = false);
	virtual ~Client(void);

	// START - PWinObj interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	virtual void iconify(void);
	virtual void stick(void);

	virtual void move(int x, int y, bool do_virtual = true);
	virtual void moveVirtual(int x, int y);
	virtual void resize(uint width, uint height);

	virtual void setWorkspace(uint workspace);

	virtual bool giveInputFocus(void);
	virtual void reparent(PWinObj *parent, int x, int y);

	virtual ActionEvent *handleButtonPress(XButtonEvent *ev) {
		if (_parent != NULL) { return _parent->handleButtonPress(ev); }
		return NULL;
	}
	virtual ActionEvent *handleButtonRelease(XButtonEvent *ev) {
		if (_parent != NULL) { return _parent->handleButtonRelease(ev); }
		return NULL;
	}

	virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev);
	// END - PWinObj interface.

	bool validate(void);

	inline std::string* getIconName(void) { return &_icon_name; }
	inline PDecor::TitleItem *getTitle(void) { return &_title; }

	inline const ClassHint* getClassHint(void) const { return _class_hint; }

	inline Window getTransientWindow(void) const { return _transient; }

	inline XSizeHints* getXSizeHints(void) const { return _size; }

	bool isViewable(void);
	bool setPUPosition(void);

	inline bool hasTitlebar(void) const { return (_state.decor&DECOR_TITLEBAR); }
	inline bool hasBorder(void) const { return (_state.decor&DECOR_BORDER); }
#ifdef HAVE_SHAPE
	inline bool isShaped(void) const { return _shaped; }
#endif // HAVE_SHAPE
	inline bool hasStrut(void) const { return (_strut); }

	// State accessors
	inline bool isMaximizedVert(void) const { return _state.maximized_vert; }
	inline bool isMaximizedHorz(void) const { return _state.maximized_horz; }
	inline bool isShaded(void) const { return _state.shaded; }
	inline bool isFullscreen(void) const { return _state.fullscreen; }
	inline bool skipTaskbar(void) const { return _state.skip_taskbar; }
	inline bool skipPager(void) const { return _state.skip_pager; }
	inline bool isPlaced(void) const { return _state.placed; }
	inline uint getSkip(void) const { return _state.skip; }
	inline uint getDecorState(void) const { return _state.decor; }
	inline bool isCfgDeny(uint deny) { return (_state.cfg_deny&deny); }

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

	void setStateCfgDeny(StateAction sa, uint deny);
	inline void setStateMarked(StateAction sa) {
        if (ActionUtil::needToggle(sa, _marked)) {
            _marked = !_marked;
            if (_marked) {
                _title.infoAdd(PDecor::TitleItem::INFO_MARKED);
            } else {
                _title.infoRemove(PDecor::TitleItem::INFO_MARKED);
            }
            _title.updateVisible();
        }
	}

	// toggles
	void alwaysOnTop(bool top);
	void alwaysBelow(bool bottom);

	void setSkip(uint skip);
	inline void setTitlebar(bool titlebar) {
		if (titlebar) {
			_state.decor |= DECOR_TITLEBAR;
		} else {
			_state.decor &= ~DECOR_TITLEBAR;
		}
	}
	inline void setBorder(bool border) {
		if (border) {
			_state.decor |= DECOR_BORDER;
		} else {
			_state.decor &= ~DECOR_BORDER;
		}
	}

#ifdef HAVE_SHAPE
	inline void setShaped(bool s) { _shaped = s; }
#endif // HAVE_SHAPE

	void close(void);
	void kill(void);

	// Event handlers below - Used by WindowManager
	void handleDestroyEvent(XDestroyWindowEvent *ev);
	void handleColormapChange(XColormapEvent *ev);

	inline bool setConfigureRequestLock(bool lock) {
		bool old_lock = _cfg_request_lock;
		_cfg_request_lock = lock;
		return old_lock;
	}
	void configureRequestSend(void);

	bool getIncSize(uint *r_w, uint *r_h, uint w, uint h);

	bool getEwmhStates(NetWMStates &win_states);
	void updateEwmhStates(void);

	void getWMNormalHints(void);
	void getWMProtocols(void);
	void getTransientForHint(void);
	void getStrutHint(void);
	void getXClientName(void);
	void getXIconName(void);
	void removeStrutHint(void);

private:
	bool titleApplyRule(std::string &title);
	uint titleFindID(std::string &title);

	void setWmState(ulong state);
	long getWmState(void);

	int sendXMessage(Window, Atom, long mask,
		long v1 = 0l, long v2 = 0l, long v3 = 0l, long v4 = 0l, long v5 = 0l);

	MwmHints* getMwmHints(Window w);

	// these are used by frame
	inline void setMaximizedVert(bool m) { _state.maximized_vert = m; }
	inline void setMaximizedHorz(bool m) { _state.maximized_horz = m; }
	inline void setShade(bool s) { _state.shaded = s; }
	inline void setFullscreen(bool f) { _state.fullscreen = f; }
	inline void setFocusable(bool f) { _focusable = f; }

	inline void setStateSkipTaskbar(StateAction sa) {
		if (((_state.skip_taskbar == true) && (sa == STATE_SET)) ||
				((_state.skip_taskbar == false) && (sa == STATE_UNSET))) {
			return;
		}
		_state.skip_taskbar = !_state.skip_taskbar;
	}
	inline void setStateSkipPager(StateAction sa) {
		if (((_state.skip_pager == true) && (sa == STATE_SET)) ||
				((_state.skip_pager == false) && (sa == STATE_UNSET))) {
			return;
		}
		_state.skip_pager = !_state.skip_pager;
	}

	// Grabs button with Caps,Num and so on
	void grabButton(int button, int mod, int mask, Window win, Cursor curs);

	void readClassRoleHints(void);
	void readEwmhHints(void);
	void readMwmHints(void);
	void readPekwmHints(void);
	AutoProperty* readAutoprops(uint type = 0);
	void applyAutoprops(AutoProperty *ap);

private: // Private Member Variables
	WindowManager *_wm;

	XSizeHints *_size;
	Colormap _cmap;

	Window _transient; // window id for which this client is transient for

	Strut *_strut;

	std::string _icon_name;
	PDecor::TitleItem _title;

	ClassHint *_class_hint;

	bool _alive, _marked;
	bool _send_focus_message, _send_close_message;
	bool _cfg_request_lock;
#ifdef HAVE_SHAPE
	bool _shaped;
#endif // HAVE_SHAPE
	bool _extended_net_name;

	class State {
	public:
		State(void) : maximized_vert(false), maximized_horz(false),
									shaded(false), fullscreen(false),
									skip_taskbar(false), skip_pager(false),
									placed(false), skip(0), decor(DECOR_TITLEBAR|DECOR_BORDER),
									cfg_deny(CFG_DENY_NO) { }
		~State(void) { }

		bool maximized_vert, maximized_horz;
		bool shaded;
		bool fullscreen;
		bool skip_taskbar, skip_pager;

		// pekwm states
		bool placed;
		uint skip, decor, cfg_deny;
	} _state;

	class Actions {
	public:
		Actions(void) : move(true), resize(true), minimize(true),
								shade(true), stick(true), maximize_horz(true),
								maximize_vert(true), fullscreen(true),
								change_desktop(true), close(true) { }
		~Actions(void) { }

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
};

#endif // _CLIENT_HH_
