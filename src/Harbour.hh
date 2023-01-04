//
// Harbour.hh for pekwm
// Copyright (C) 2003-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_HARBOUR_HH_
#define _PEKWM_HARBOUR_HH_

#include "config.h"

#include "pekwm.hh"

class AutoProperties;
class Config;
class DockApp;
class RootWO;
class Strut;

#include "Action.hh"

class Harbour
{
public:
	Harbour(Config* cfg, AutoProperties* ap, RootWO *root_wo);
	~Harbour(void);

	void addDockApp(DockApp* da);
	void removeDockApp(DockApp* da);
	void removeAllDockApps(void);

	DockApp* findDockApp(Window win);
	DockApp* findDockAppFromFrame(Window win);

	inline uint getSize(void) const { return _size; }

	void updateGeometry(void);

	void restack(void);
	void rearrange(void);
	void loadTheme(void);
	void updateHarbourSize(void);

	void setStateHidden(StateAction sa);

	void handleButtonEvent(XButtonEvent* ev, const DockApp* da);
	void handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da);
	void handleConfigureRequestEvent(XConfigureRequestEvent* ev,
					 DockApp* da);

private:
	Harbour(const Harbour&);
	Harbour& operator=(const Harbour&);

	void placeDockApp(DockApp *da);
	void placeDockAppX(DockApp *da, const Geometry& head,
			   int& x, const int y);
	void placeDockAppsSorted(void);
	void placeDockAppInsideScreen(DockApp *da);

	void getPlaceStartPosition(DockApp *da, int &x, int &y, bool &inc_x);
	void insertDockAppSorted(DockApp *da);

	void updateStrutSize(void);

private:
	Config* _cfg;
	AutoProperties* _ap;
	RootWO *_root_wo;

	std::vector<DockApp*> _dapps;
	bool _hidden;
	uint _size;
	Strut *_strut;
	int _last_button_x, _last_button_y;
};

namespace pekwm
{
	Harbour* harbour();
}

#endif // _PEKWM_HARBOUR_HH_
