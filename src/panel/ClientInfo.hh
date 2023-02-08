//
// ClientInfo.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_CLIENT_INFO_HH
#define _PEKWM_PANEL_CLIENT_INFO_HH

#include "../tk/PImage.hh"
#include "../tk/PImageIcon.hh"
#include "../tk/X11Util.hh"

class ClientInfo : public NetWMStates {
public:
	ClientInfo(Window window);
	virtual ~ClientInfo(void);

	Window getWindow(void) const { return _window; }
	const std::string& getName(void) const { return _name; }
	const Geometry& getGeometry(void) const { return _gm; }
	PImage *getIcon(void) const { return _icon; }

	bool displayOn(uint workspace) const
	{
		return sticky || this->_workspace == workspace;
	}

	bool handlePropertyNotify(XPropertyEvent *ev);

private:
	std::string readName(void);
	Geometry readGeometry(void) { return Geometry(); }
	uint readWorkspace(void);

private:
	Window _window;
	std::string _name;
	Geometry _gm;
	uint _workspace;
	PImageIcon *_icon;
};

#endif // _PEKWM_PANEL_CLIENT_INFO_HH
