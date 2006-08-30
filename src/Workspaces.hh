//
// Workspaces.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _WORKSPACES_HH_
#define _WORKSPACES_HH_

#include "pekwm.hh"

#include <string>
#include <list>
#include <vector>

class ScreenInfo;
class EwmhAtoms;
class Config;
class WindowObject;
class Frame;
class FrameWidget;

class Workspaces {
public:
	class Workspace {
	public:
		Workspace(std::string name, unsigned int number) :
			_name(name), _number(number), _last_focused(NULL) { }
		~Workspace() { }

		inline std::string& getName(void) { return _name; }
		inline unsigned int getNumber(void) { return _number; }
		inline WindowObject* getLastFocused(void) { return _last_focused; }

		inline void setLastFocused(WindowObject* wo) { _last_focused = wo; }

	private:
		std::string _name;
		unsigned int _number;

		WindowObject *_last_focused;
	};

	Workspaces(ScreenInfo* scr, Config* cfg, EwmhAtoms* ewmh,
						 unsigned int number);
	~Workspaces();

	inline unsigned int getActive(void) const { return _active; }
	inline unsigned int getNumber(void) const { return _workspace_list.size(); }
	void setNumber(unsigned int number);

	inline const std::list<WindowObject*> &getWOList(void) const {
		return _wo_list; }

	void insert(WindowObject* wo, bool raise = true);
	void remove(WindowObject* wo);

	void hideAll(unsigned int workspace);
	void unhideAll(unsigned int workspace, bool focus);

	WindowObject* getLastFocused(unsigned int workspace);
	void setLastFocused(unsigned int workspace, WindowObject* wo);

	void raise(WindowObject* wo);
	void lower(WindowObject* wo);
	void stackAbove(WindowObject* wo, Window win, bool restack = true);
	void stackBelow(WindowObject *wo, Window win, bool restack = true);

	WindowObject* getTopWO(unsigned int type_mask);
	void updateClientStackingList(bool client, bool stacking);
	void checkFrameSnap(Geometry& f_gm, Frame* frame);
	void placeFrame(Frame* frame, Geometry& gm);

private:
	void stackWinUnderWin(Window over, Window under);

	// placement
	bool placeSmart(WindowObject* wo, Geometry& gm);
	bool placeCenteredUnderMouse(Geometry& gm);
	bool placeTopLeftUnderMouse(Geometry& gm);
	void placeInsideScreen(Geometry& gm);

	// placement helpers
	WindowObject* isEmptySpace(int x, int y,
														 const WindowObject* wo, const Geometry& gm);
	inline bool isBetween(const int &x1, const int &x2,
												const int &t1, const int &t2) {
		if (x1 > t1) {
			if (x1 < t2)
				return true;
		} else if (x2 > t1) {
			return true;
		}
		return false;
	}

private:
	ScreenInfo *_scr;
	Config *_cfg;
	EwmhAtoms *_ewmh;

	unsigned int _active;

	std::list<WindowObject*> _wo_list;
	std::vector<Workspace*> _workspace_list;
};

#endif // _WORKSPACES_HH_
