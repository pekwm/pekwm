//
// ManagerWindows.hh for pekwm
// Copyright (C) 2009-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_MANAGERWINDOWS_HH_
#define _PEKWM_MANAGERWINDOWS_HH_

#include "config.h"
#include "PWinObj.hh"
#include "pekwm.hh"

#include <string>

class Config;

/**
 * Window for handling of EWMH hints, sets supported attributes etc.
 */
class HintWO : public PWinObj
{
public:
	HintWO(Window root);
	virtual ~HintWO(void);

	bool claimDisplay(bool replace);

private:
	Time getTime(void);
	bool claimDisplayWait(Window session_owner);
	void claimDisplayOwner(Window session_atom, Time timestamp);

private:
	/** Name of the window manager, that is pekwm. */
	static const std::string WM_NAME;
	/** Max wait time for previous WM. */
	static const unsigned int DISPLAY_WAIT;
};

/**
 * Window object representing the Root window, handles actions and
 * sets atoms on the window.
 */
class RootWO : public PWinObj
{
public:
	RootWO(Window root, HintWO *hint_wo, Config *cfg);
	virtual ~RootWO(void);

	/** Resize root window, does no actual resizing but updates the
	    geometry of the window. */
	virtual void resize(uint width, uint height) {
		_gm.width = width;
		_gm.height = height;
	}

	virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
	virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
	virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
	virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
	virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

	void placeInsideScreen(Geometry &gm,
			       bool without_edge=false,
			       bool fullscreen=false,
			       bool maximized_vert=false,
			       bool maximized_horz=false);
	void getHeadInfoWithEdge(uint num, Geometry& head);

	void updateGeometry(uint width, uint height);
	void addStrut(Strut *strut);
	void removeStrut(Strut *rem_strut);
	void updateStrut(void);
	const Strut& getStrut(void) { return _strut; }
	const Strut& getStrut(uint head) {
		if (head < _strut_head.size()) {
			return _strut_head[head];
		}
		return _strut;
	}

	void handlePropertyChange(XPropertyEvent *ev);

	void setEwmhWorkarea(const Geometry &workarea);
	void setEwmhActiveWindow(Window win);
	void readEwmhDesktopNames(void);
	void setEwmhDesktopNames(void);
	void setEwmhDesktopLayout(void);

private:
	void initStrutHead();
	void updateMaxStrut(Strut *max_strut, const Strut *strut);

private:
	HintWO *_hint_wo;
	Config *_cfg;

	Strut _strut;
	std::vector<Strut> _strut_head;
	std::vector<Strut*> _struts;

	/** Root window event mask. */
	static const unsigned long EVENT_MASK;
	/** Expected length of desktop hint. */
	static const unsigned long EXPECTED_DESKTOP_NAMES_LENGTH;
};

/**
 * Window object used as a screen border, input only window that only
 * handles actions.
 */
class EdgeWO : public PWinObj
{
public:
	EdgeWO(RootWO *root_wo, EdgeType edge, bool set_strut,
	       Config *cfg);
	virtual ~EdgeWO(void);

	void configureStrut(bool set_strut);

	virtual void mapWindow(void);

	virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
	virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
	virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);

	inline EdgeType getEdge(void) const { return _edge; }

private:
	RootWO* _root;
	EdgeType _edge; /**< Edge position. */
	Config *_cfg;
	Strut _strut; /*< Strut for reserving screen edge space. */
};

namespace pekwm
{
	RootWO* rootWo();
}

#endif // _PEKWM_MANAGERWINDOWS_HH_
