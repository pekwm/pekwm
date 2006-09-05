//
// atoms.cc for pekwm
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

#include "atoms.hh"

IcccmAtoms::IcccmAtoms(Display *dpy)
{
	// ICCCM atoms
	wm_state = XInternAtom(dpy, "WM_STATE", 0);
	wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", 0);
	wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", 0);
	wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);
	wm_cmapwins = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", 0);
	wm_takefocus = XInternAtom(dpy, "WM_TAKE_FOCUS", 0);
}

IcccmAtoms::~IcccmAtoms()
{
}

EwmhAtoms::EwmhAtoms(Display *dpy)
{
	net_supported = XInternAtom(dpy, "_NET_SUPPORTED", 0);
	net_client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", 0);
	net_client_list_stacking = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", 0);
	net_number_of_desktops = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", 0);
	net_desktop_geometry = XInternAtom(dpy, "_NET_DESKTOP_GEOMETRY", 0);
	net_desktop_viewport = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", 0);
	net_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", 0);
	//net_desktop_names =
	//  XInternAtom(dpy, "_NET_DESKTOP_NAMES", 0);
	net_active_window = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", 0);
	net_workarea = XInternAtom(dpy, "_NET_WORKAREA", 0);
	net_supporting_wm_check = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", 0);
	//net_virtual_roots = XInternAtom(dpy, "_NET_VIRTUAL_ROOTS", 0);
	net_close_window = XInternAtom(dpy, "_NET_CLOSE_WINDOW", 0);
	//net_moveresize_window = XInternAtom(dpy, "_NET_MOVERESIZE_WINDOW", 0);
	//net_wm_moveresize = XInternAtom(dpy, "_NET_WM_MOVERESIZE", 0);
	net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", 0);
	//net_wm_visible_name = XInternAtom(dpy, "_NET_WM_VISIBLE_NAME", 0);
	//net_wm_icon_name = XInternAtom(dpy, "_NET_WM_ICON_NAME", 0);
	//net_wm_visible_icon_name = XInternAtom(dpy, "_NET_WM_VISIBLE_ICON_NAME", 0);
	net_wm_desktop = XInternAtom(dpy, "_NET_WM_DESKTOP", 0);

	// wm type atoms
	net_wm_window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", 0);
	net_wm_window_type_desktop =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
	net_wm_window_type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", 0);
	net_wm_window_type_toolbar =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_TOOLBAR", 0);
	net_wm_window_type_menu = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_MENU", 0);
	net_wm_window_type_utility =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_UTILITY", 0);
	net_wm_window_type_splash =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_SPLASH", 0);
	net_wm_window_type_dialog =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", 0);
	net_wm_window_type_normal =
		XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_NORMAL", 0);

	// wm state atoms
	net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", 0);
	net_wm_state_modal = XInternAtom(dpy, "_NET_WM_STATE_MODAL", 0);
	net_wm_state_sticky = XInternAtom(dpy, "_NET_WM_STATE_STICKY", 0);
	net_wm_state_maximized_vert =
		XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
	net_wm_state_maximized_horz =
		XInternAtom(dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);
	net_wm_state_shaded = XInternAtom(dpy, "_NET_WM_STATE_SHADED", 0);
	net_wm_state_skip_taskbar =
		XInternAtom(dpy, "_NET_WM_STATE_SKIP_TOOLBAR", 0);
	net_wm_state_skip_pager = XInternAtom(dpy, "_NET_WM_STATE_PAGER", 0);
	net_wm_state_hidden = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", 0);
	net_wm_state_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", 0);

	net_wm_strut = XInternAtom(dpy, "_NET_WM_STRUT", 0);

	//net_wm_icon_geometry = XInternAtom(dpy, "_NET_WM_ICON_GEOMETRY", 0);
	//net_wm_icon = XInternAtom(dpy, "_NET_WM_ICON", 0);
	//net_wm_pid = XInternAtom(dpy, "_NET_WM_PID", 0);
	//net_wm_handled_icons = XInternAtom(dpy, "_NET_WM_HANDLED_ICONS", 0);
	//net_wm_ping = XInternAtom(dpy, "_NET_WM_PING", 0);
}

EwmhAtoms::~EwmhAtoms()
{
}
