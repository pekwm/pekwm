//
// Frame.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _FRAME_HH_
#define _FRAME_HH_

#include "pekwm.hh"
#include "Action.hh"

class ScreenInfo;
class WindowObject;
class Strut;
class Theme;
class WindowManager;
class ClassHint;

class FrameWidget;
class Button;
class Client;

#include <list>
#include <vector>

class Frame : public WindowObject
{
public:
	Frame(WindowManager *w, Client *cl);
	~Frame();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	virtual void iconify(void);

	virtual void setWorkspace(unsigned int workspace);
	virtual void setFocused(bool focused);

	virtual void giveInputFocus(void);
	// END - WindowObject interface.

	unsigned int getId(void) const { return _id; }
	void setId(unsigned int id);

	void loadTheme(void);

	void insertClient(Client *client, bool focus = true, bool activate = true);
	void removeClient(Client *client);
	void detachClient(Client *client);

	void insertFrame(Frame* frame, bool focus = true, bool activate = true);

	void activateClient(Client *client, bool focus = true);
	void activateNextClient(void);
	void activatePrevClient(void);
	void activateClientFromPos(int x);
	void activateClientFromNum(unsigned int n);

	void moveClientNext(void);
	void moveClientPrev(void);

	inline FrameWidget* getFrameWidget(void) { return _fw; }
	inline unsigned int getNumClients(void) const { return _client_list.size(); }
	inline std::vector<Client*>* getClientList(void) { return &_client_list; }

	bool clientIsInFrame(Client *c);
	Client *getClientFromPos(unsigned int x);

	inline Client* getActiveClient(void) const { return _client; }
	inline bool isActiveClient(Client *c) const { return (_client == c); }

	inline const ClassHint* getClassHint(void) const { return _class_hint; }

	inline bool isShaded(void) const { return _state.shaded; }
	inline bool isSkip(unsigned int skip) const { return (_state.skip&skip); }

	void toggleBorder(bool resize = true);
	void toggleTitlebar(bool resize = true);
	void toggleDecor(void);

	void moveToEdge(Edge edge);
	void resizeHorizontal(int size_diff);
	void resizeVertical(int size_diff);
	void raise(void);
	void lower(void);
	void shade(void);
	void stick(void);
	void maximize(bool horz, bool vert);

	void alwaysOnTop(void);
	void alwaysBelow(void);

	void readAutoprops(unsigned int type = APPLY_ON_RELOAD);

	void doMove(XMotionEvent *ev);
	void doResize(XMotionEvent *ev); // redirects to doResize(bool...
	void doResize(bool left, bool x, bool top, bool y);
	void doGroupingDrag(XMotionEvent *ev, Client *client, bool behind);
#ifdef KEYS
	void doKeyboardMoveResize(void);
#endif // KEYS

#ifdef MENUS
	void showWindowMenu(void);
#endif // MENUS

	bool fixGeometry(bool harbour);

	void updateSize(void);
	void updateSize(unsigned int w, unsigned int h);
	void updateClientSize(Client *client);
	void updatePosition(void);
	void updatePosition(int x, int y);

	// event handlers
	ActionEvent* handleButtonEvent(XButtonEvent *e);
	ActionEvent* handleMotionNotifyEvent(XMotionEvent *ev);
	// gravitation
	void gravitate(Client *client, bool apply);
	// client message handling
	void handleConfigureRequest(XConfigureRequestEvent *ev, Client *client);
	void handleClientMessage(XClientMessageEvent *ev, Client *client);
	void handlePropertyChange(XPropertyEvent *ev, Client *client);

private:
	void recalcResizeDrag(int nx, int ny, bool left, bool top);
	void checkEdgeSnap(void);
	void calcSizeInCells(unsigned int &width, unsigned int &height);

	ActionEvent* findMouseButtonAction(unsigned int button, unsigned int mod,
																		 MouseButtonType type,
																		 std::list<ActionEvent>* actions);
	void getState(Client *cl);
	void applyState(Client *cl);

	void updateTitles(void);
	void setActiveTitle(void);
private:
	WindowManager *_wm;
	ScreenInfo *_scr;
	Theme *_theme;

	unsigned int _id; // unique id of the frame

	FrameWidget *_fw;

	std::vector<Client*> _client_list;
	std::list<Button*> _button_list;

	Client *_client;
	Button *_button;
	ClassHint *_class_hint;

	// frame position and size
	Geometry _old_gm;

	int _pointer_x, _pointer_y; // Used in client move
	int _old_cx, _old_cy;
	unsigned int _real_height; // Used when shading

	// state switches specific for the frame
	class State {
	public:
		State() : maximized_vert(false), maximized_horz(false), shaded(false),
							skip(0) { }

		bool maximized_vert, maximized_horz;
		bool shaded;

		unsigned int skip;
	} _state;

	// EWMH
	static const int NET_WM_STATE_REMOVE = 0; // remove/unset property
	static const int NET_WM_STATE_ADD = 1; // add/set property
	static const int NET_WM_STATE_TOGGLE = 2; // toggle property
};

#endif // _FRAME_HH_
