//
// atoms.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
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

#ifndef _ATOMS_HH_
#define _ATOMS_HH_

#include <X11/Xlib.h>

// ICCCM stuff
class IcccmAtoms {
public:
	IcccmAtoms(Display *dpy);
	~IcccmAtoms();
public:
	Atom wm_state,
		wm_change_state,
		wm_protos,
		wm_delete,
		wm_cmapwins,
		wm_takefocus;
};

// Extended WM Hints
class EwmhAtoms {
public:
	EwmhAtoms(Display *dpy);
	~EwmhAtoms();
public:
	Atom net_supported,
		net_client_list,
		net_client_list_stacking,
		net_number_of_desktops,
		net_desktop_geometry,
		net_desktop_viewport,
		net_current_desktop,
		//net_desktop_names,
		net_active_window,
		net_workarea,
		net_supporting_wm_check,
		//net_virtual_roots,
		net_close_window,
		//net_moveresize_window,
		//net_wm_moveresize,
		net_wm_name,
		//net_wm_visible_name,
		//net_wm_icon_name,
		//net_wm_visible_icon_name,
		net_wm_desktop,
		net_wm_window_type,
		net_wm_window_type_desktop,
		net_wm_window_type_dock,
		net_wm_window_type_toolbar,
		net_wm_window_type_menu,
		net_wm_window_type_utility,
		net_wm_window_type_splash,
		net_wm_window_type_dialog,
		net_wm_window_type_normal,
		net_wm_state,
		net_wm_state_modal,
		net_wm_state_sticky,
		net_wm_state_maximized_vert,
		net_wm_state_maximized_horz,
		net_wm_state_shaded,
		net_wm_state_skip_taskbar,
		net_wm_state_skip_pager,
		net_wm_state_hidden,
		net_wm_state_fullscreen,
		net_wm_strut;
	//net_wm_icon_geometry,
	//net_wm_icon,
	//net_wm_pid,
	//net_wm_handled_icons,
	//net_wm_ping;
};

#endif // _FONT_HH_
