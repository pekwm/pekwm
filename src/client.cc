//
// client.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// client.cc for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
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

#include "client.hh"
#include "frame.hh"
#include "windowmanager.hh"

#include <X11/Xatom.h>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::vector;
using std::list;

const int NET_WM_STATE_REMOVE = 0; // remove/unset property
const int NET_WM_STATE_ADD = 1; // add/set property
const int NET_WM_STATE_TOGGLE = 2; // toggle property

const int APPLY_GRAVITY = 1;
const int REMOVE_GRAVITY = -1;

Client::Client(WindowManager *w, Window new_client) :
wm(w),
m_window(new_client), m_trans(None),
m_frame(NULL),
m_x(0), m_y(0), m_width(1), m_height(1),
m_has_focus(false), m_has_titlebar(true), m_has_border(true),
m_is_shaded(false), m_is_iconified(false), m_is_maximized(false),
m_is_maximized_vertical(false), m_is_maximized_horizontal(false),
m_is_sticky(false), m_is_hidden(false),
m_win_layer(WIN_LAYER_NORMAL),
#ifdef SHAPE
m_has_been_shaped(false),
#endif // SHAPE
m_client_strut(NULL), m_has_strut(false),
m_has_extended_net_name(false), m_skip_taskbar(false), m_skip_pager(false),
m_is_alive(true)
{
	dpy = wm->getScreen()->getDisplay();

	// make new client
	constructClient();

	// if this is set, then we found a group to auto-insert this client in
	if (m_frame) {
		m_frame->insertClient(this, true);
	} else {	// we now have a client, let's give it a frame
		m_frame = new Frame(wm, this);
	}

	// We don't add it anywhere before we get here, as it can change while
	// loading autoprops and so on
	wm->getWorkspaces()->addClient(this, m_on_workspace);
	setWorkspace(m_on_workspace);

	// to get the correct stacking
	if (m_win_layer < WIN_LAYER_NORMAL)
		m_frame->lower();
	else
		m_frame->raise();

	// finished creating the client, so now adding it to the client list
	wm->addToClientList(this);
	wm->getWorkspaces()->updateClientList();
}

Client::~Client()
{
	XGrabServer(dpy);

	// I'm doing this before the m_frame->removeClient() as that'll result in
	// a updateClientList when we still have these pointers hanging around
	// causing a segfault
	wm->getWorkspaces()->removeClient(this, m_on_workspace);
	if (m_is_sticky)
		wm->getWorkspaces()->unstickClient(this);
#ifdef MENUS
	if (m_is_iconified)
		wm->removeClientFromIconMenu(this);
#endif // MENUS

	wm->removeFromClientList(this);
	m_frame->removeClient(this);

	// Focus the parent if we had focus before
	if(m_has_focus && m_trans) {
		Client *parent = wm->findClientFromWindow(m_trans);
		if (parent)
			parent->giveInputFocus();
	}

	// Clean up if the client still is alive, it'll be dead all times
	// except when we exit pekwm
	if (m_is_alive) {
		XUngrabButton(dpy, AnyButton, AnyModifier, m_window);
#ifdef KEYS
		wm->getKeys()->ungrabKeys(m_window);
#endif // KEYS

		// move the window back to root
		gravitate(REMOVE_GRAVITY);
		XReparentWindow(dpy, m_window, wm->getScreen()->getRoot(), m_x, m_y);

		XRemoveFromSaveSet(dpy, m_window);
	}

	// free names and size hint
	if (m_size)
		XFree(m_size);

	if (m_client_strut) {
		if(m_has_strut) {
			wm->removeStrut(m_client_strut);
		}
		delete m_client_strut;
		m_client_strut = NULL;
	}

	wm->getWorkspaces()->updateClientList();

	XSync(dpy, False);
	XUngrabServer(dpy);
}

// Set up a client structure for the new (not-yet-mapped) window. The
// confusing bit is that we have to ignore 2 unmap events if the
// client was already mapped but has Iconic State set (for instance,
// when we are the second window manager in a session).  That's
// because there's one for the re-parent (which happens on all viewable
// windows) and then another for the unmapping itself.
void
Client::constructClient(void)
{
	XGrabServer(dpy);

	XWMHints* hints;
	MwmHints* mhints;

	long dummy;

	// get trans window and the window attributes
	XGetTransientForHint(dpy, m_window, &m_trans);

	XSetWindowAttributes sattr;
	sattr.event_mask =
		PropertyChangeMask|StructureNotifyMask|FocusChangeMask;
	sattr.do_not_propagate_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

	// We don't want these masks to be propagated down to the frame
	XChangeWindowAttributes(dpy, m_window, CWEventMask|CWDontPropagate, &sattr);
	//	XSelectInput(dpy, m_window, FocusChangeMask);

	XAddToSaveSet(dpy, m_window);
	XSetWindowBorderWidth(dpy, m_window, 0);

#ifdef SHAPE
	if (wm->hasShape())
		XShapeSelectInput(dpy, m_window, ShapeNotifyMask);
#endif // SHAPE

	XWindowAttributes attr;
	XGetWindowAttributes(dpy, m_window, &attr);
	m_x = attr.x;
	m_y = attr.y;
	m_width = attr.width;
	m_height = attr.height;

	m_cmap = attr.colormap;
	m_size = XAllocSizeHints();

	XGetWMNormalHints(dpy, m_window, m_size, &dummy);
	if ((mhints = getMwmHints(m_window))) {
		if ((mhints->flags&MWM_HINTS_DECORATIONS) &&
				!(mhints->decorations&MWM_DECOR_ALL)) {
			m_has_titlebar = mhints->decorations&MWM_DECOR_TITLE;
			m_has_border = mhints->decorations&MWM_DECOR_BORDER;
		}
		XFree(mhints);
	}

	getXClientName();
	getXIconName();

	initHintProperties();

	// Setup the clients position
	if (!setPUPosition() && (attr.map_state != IsViewable)) {
		if ((m_win_layer != WIN_LAYER_DESKTOP) && (m_win_layer < WIN_LAYER_DOCK)) {
			bool placed = false;
			switch(wm->getConfig()->getPlacementModel()) {
			case SMART:
				placed = placeSmart();
				break;
			case MOUSE_CENTERED:
				placed = placeCenteredUnderMouse();
				break;
			case MOUSE_TOP_LEFT:
				placed = placeTopLeftUnderMouse();
				break;
			default:
				// do nothing
				break;
			}

			if (!placed) {
				switch(wm->getConfig()->getFallbackPlacementModel()) {
				case SMART:
					placeSmart();
					break;
				case MOUSE_CENTERED:
					placeCenteredUnderMouse();
					break;
				case MOUSE_TOP_LEFT:
					placeTopLeftUnderMouse();
					break;
				default:
					// do nothing
					break;
				}
			}
		}
	}

	// Load the Class hint before loading the autoprops
	XClassHint class_hint;
	if (XGetClassHint(dpy, m_window, &class_hint)) {
		m_class_hint.setName(class_hint.res_name);
		m_class_hint.setClass(class_hint.res_class);
		XFree(class_hint.res_name);
		XFree(class_hint.res_class);
	}

	if (attr.map_state == IsViewable) {
		//TO-DO: Hmm, I don't have a frame...
		m_x -= m_has_border
			? wm->getTheme()->getWinUnfocusedBorder()[BORDER_LEFT]->getWidth()
			: 0;
		m_y -= m_has_border
			? wm->getTheme()->getWinUnfocusedBorder()[BORDER_TOP]->getHeight()
			: 0;

		loadAutoProps(AutoProps::APPLY_ON_START);
	} else {
		loadAutoProps(-1);
	}

	gravitate(APPLY_GRAVITY);

	if ((hints = XGetWMHints(dpy, m_window))) {
		if (hints->flags&StateHint) {
			setWmState(hints->initial_state);
		} else {
			setWmState(NormalState);
		}
		XFree(hints);
	}

	updateWmStates();

	// if we aren't allready iconified, look and see
	if (!m_is_iconified) {
		if ((getWmState() == NormalState) || (getWmState() == WithdrawnState)) {
			// This makes an extra XMoveWindow, but makes it less flickery
			XMoveWindow(dpy, m_window, m_x, m_y);
			XMapWindow(dpy, m_window);

			m_is_iconified = false;
		} else if (getWmState() == IconicState) {
			m_is_iconified = true;
		}
	}

#ifdef KEYS
	wm->getKeys()->grabKeys(m_window);
#endif // KEYS

	list<MouseButtonAction> *actions =
		wm->getConfig()->getMouseClientList();
	list<MouseButtonAction>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if (it->type == BUTTON_SINGLE) {
			// No need to grab mod less events, replied with the frame
			if (!it->mod)
				continue;

			grabButton(it->button, it->mod,
								 ButtonPressMask|ButtonReleaseMask,
								 m_window, None);
		} else if (it->type == BUTTON_MOTION) {
			grabButton(it->button, it->mod,
								 ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
								 m_window, None);
		}
	}

	XSync(dpy, False);
	XUngrabServer(dpy);
}

void
Client::initHintProperties(void)
{
	EwmhAtoms *ewmh = wm->getEwmhAtoms(); // convinience

	// which workspace do we belong to?
	if ((m_on_workspace = wm->findDesktopHint(m_window)) == -1) {
		m_on_workspace = wm->getActiveWorkspace();
		wm->setExtendedWMHint(m_window, ewmh->net_wm_desktop, m_on_workspace);
	}

	NetWMStates win_states;
	wm->getExtendedNetWMStates(m_window, win_states);

	if (!m_is_sticky && win_states.sticky) {
		m_is_sticky = true;
		wm->getWorkspaces()->stickClient(this);
	}
	if (win_states.shaded) m_is_shaded = true;
	if (win_states.max_vertical) m_is_maximized_vertical = true;
	if (win_states.max_horizontal) m_is_maximized_horizontal = true;
	if (win_states.skip_taskbar) m_skip_taskbar = true;
	if (win_states.skip_pager) m_skip_pager = true;
	if (win_states.hidden) m_is_iconified = true;
	// TO-DO: Add support for net_wm_state_fullscreen

	// Do we want a strut?
	int num = 0;
	CARD32 *strut = NULL;
	strut = (CARD32*) wm->getExtendedNetPropertyData(
    m_window, ewmh->net_wm_strut, XA_CARDINAL, &num);

	if (strut) {
		if (! m_client_strut)
			m_client_strut = new Strut;

		m_client_strut->west = strut[0];
		m_client_strut->east = strut[1];
		m_client_strut->north = strut[2];
		m_client_strut->south = strut[3];

		wm->addStrut(m_client_strut);

		m_has_strut = true;

		XFree(strut);
	}

	// Try to figure out what kind of window we are and alter it acordingly
	int items;
	Atom *atoms = NULL;

	atoms = (Atom*)
		wm->getExtendedNetPropertyData(m_window, ewmh->net_wm_window_type,
																	 XA_ATOM, &items);

	if (atoms) {
		bool found_window_type = false;

		for (int i = 0; i < items; ++i) {
			found_window_type = true;

			if (atoms[i] == ewmh->net_wm_window_type_desktop) {
				// desktop windows, make it the same size as the screen and place
				// it below all windows withouth any decorations, also make it sticky
				m_x = m_y = 0;
				m_width = wm->getScreen()->getWidth();
				m_height = wm->getScreen()->getHeight();
				m_has_titlebar = false;
				m_has_border = false;
				if (!m_is_sticky) {
					m_is_sticky = true;
					wm->getWorkspaces()->stickClient(this);
				}
				m_win_layer = WIN_LAYER_DESKTOP;
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_dock) {
				m_has_titlebar = false;
				m_has_border = false;
				if (!m_is_sticky) {
					m_is_sticky = true;
					wm->getWorkspaces()->stickClient(this);
				}
				m_win_layer = WIN_LAYER_DOCK;
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_toolbar) {
				m_has_titlebar = false;
				m_has_border = false;
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_menu) {
				m_win_layer = WIN_LAYER_MENU;
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_utility) {
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_splash) {
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_dialog) {
				break;
			} else if (atoms[i] == ewmh->net_wm_window_type_normal) {
				break;
			} else {
				found_window_type = false;
			}
		}

		if (!found_window_type) {
			Atom type = ewmh->net_wm_window_type_normal;
			XChangeProperty(dpy, m_window, ewmh->net_wm_window_type, XA_ATOM, 32,
											PropModeReplace, (unsigned char *) &type, 1);
		}


		XFree(atoms);
	} else {
		Atom type = ewmh->net_wm_window_type_normal;
		XChangeProperty(dpy, m_window, ewmh->net_wm_window_type, XA_ATOM, 32,
										PropModeReplace, (unsigned char *) &type, 1);
	}
}

//! @fn    void loadAutoProps(int type)
//! @brief Tries to find a AutoProp for the current client.
void
Client::loadAutoProps(int type)
{
	if (!m_class_hint.getName().size())
		return; // we don't have  a property name to match against

	AutoProps::AutoPropData *data =
		wm->getAutoProps()->getAutoProp(m_class_hint, m_on_workspace, type);

	if (!data)
		return; // no property matching

	if (m_trans && data->applyOnTransient() && !data->apply_on_transient)
		return; // don't apply on transient

	// Set the correct group of the window
	m_class_hint.setGroup(data->class_hint.getGroup());

	// We only apply grouping if it's a new client or if we are restarting
	// and have APPLY_ON_START set
	if ((type == -1) || (type == AutoProps::APPLY_ON_START)) {
		if (data->setSticky() && (m_is_sticky != data->sticky)) {
			m_is_sticky = m_is_sticky ? false : true;
			if (m_is_sticky)
				wm->getWorkspaces()->stickClient(this);
			else
				wm->getWorkspaces()->unstickClient(this);
		}
		if (data->setShaded()) m_is_shaded = data->shaded;
		if (data->setMaximizedVertical())
			m_is_maximized_vertical = data->maximized_vertical;
		if (data->setMaximizedHorizontal())
			m_is_maximized_horizontal = data->maximized_horizontal;
		if (data->setIconified()) m_is_iconified = data->iconified;
		if (data->setBorder()) m_has_border = data->border;
		if (data->setTitlebar()) m_has_titlebar = data->titlebar;
		if (data->setPosition()) {
			m_x = data->x;
			m_y = data->y;
		}
		if (data->setSize()) {
			m_width = data->width;
			m_height = data->height;
		}
		if (data->setLayer() && (data->layer <= WIN_LAYER_MENU)) {
			m_win_layer = data->layer;
		}
		if (data->setWorkspace()) {
			if (data->workspace < wm->getWorkspaces()->getNumWorkspaces()) {
				// no need to do anything about this here, it'll be set later
				// in constructClient
				m_on_workspace = data->workspace;
			}
		}
		if (data->autoGroup() && (data->auto_group> 1)) {
			m_frame = wm->findGroup(m_class_hint, m_on_workspace, data->auto_group);
		}

		// if we are reloading or switching workspace we handle the properties
		// in another way. some properties such as size will only be applied
		// if the client are the active one in the frame.
	} else if ((type == AutoProps::APPLY_ON_RELOAD) ||
						 (type == AutoProps::APPLY_ON_WORKSPACE_CHANGE)) {

		if (data->setSticky() && (m_is_sticky != data->sticky))
			stick();
		if (data->setIconified() && (m_is_iconified != data->iconified)) {
			if (m_is_iconified)
				m_frame->unhideClient(this);
			else
				iconify();
		}
		if (data->setWorkspace() && (m_on_workspace != signed(data->workspace)))
			setWorkspace(data->workspace);

		// Some stuff we only care about if we are the active client
		if (this == m_frame->getActiveClient()) {
			if (data->setShaded() && (m_is_shaded != data->shaded))
				m_frame->shade();
			if (data->setLayer() && (data->layer <= WIN_LAYER_MENU)) {
				m_win_layer = data->layer;
				m_frame->raise(); // to do some restacking
			}
			if (data->setPosition()) {
				m_frame->updateFramePosition(data->x, data->y);
				m_frame->updateClientPosition();
			}
			if (data->setSize()) {
				m_frame->updateFrameSize(data->width, data->height);
				m_frame->updateClientGeometry();
			}
			if (data->setBorder() && (m_has_border != data->border))
				toggleBorder();
			if (data->setTitlebar() && (m_has_titlebar != data->titlebar))
				toggleTitlebar();
		}
	}
}

//! @fn    unsigned int calcTitleHeight(void)
//! @brief Returns the desired height of the title
unsigned int
Client::calcTitleHeight(void)
{
	unsigned int title_height = 0;
	if (m_has_titlebar) {
		if (m_trans) {
			title_height = wm->getTheme()->getWinTitleHeight() / 2;
		} else {
			title_height = wm->getTheme()->getWinTitleHeight();
		}
	}

	return title_height;
}

//! @fn    bool placeSmart(void)
//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
bool
Client::placeSmart(void)
{
	ScreenInfo *scr = wm->getScreen(); // convenience

	// the title height ... don't have any frame yet soo...
	unsigned int title_height = calcTitleHeight();

	// settings
	bool placed = false, increase = false;
	int	step_x = 1, step_y = 1;

	unsigned int brdr_tb =
		wm->getTheme()->getWinUnfocusedBorder()[BORDER_TOP]->getHeight()+
		wm->getTheme()->getWinUnfocusedBorder()[BORDER_BOTTOM]->getHeight();

	unsigned int win_w, win_h;
	getFrameSize(win_w, win_h);

	// we create a list of all clients on the workspace, including the sticky
	// ones. so that the testing code hopefully will becom a bit cleaner
	list<Client*>
		client_list(wm->getWorkspaces()->getClientList(m_on_workspace)->begin(),
								wm->getWorkspaces()->getClientList(m_on_workspace)->end());
	client_list.insert(client_list.end(),
										 wm->getWorkspaces()->getStickyClientList()->begin(),
										 wm->getWorkspaces()->getStickyClientList()->end());

#ifdef XINERAMA
	ScreenInfo::HeadInfo head;
	unsigned int head_nr = 0;

	if (scr->hasXinerama())
		head_nr = scr->getCurrHead();
	scr->getHeadInfo(head_nr, head);
#endif // XINERAMA

	list<Client*>::iterator it;
	register int test_x, test_y, curr_x, curr_y, curr_w, curr_h;

#ifdef XINERAMA
	test_x = head.x;
	while ((test_x + win_w) < (head.x + head.width) && !placed) {
		test_y = head.y;
		while ((test_y + win_h) < (head.y + head.height) && !placed) {
#else // !XINERAMA
	test_x = wm->getMasterStrut()->west;
	while ( (test_x+win_w) < scr->getWidth() && !placed) {
		test_y = wm->getMasterStrut()->north;
		while ((test_y+win_h) < scr->getHeight() && !placed) {
#endif // !XINERAMA

			// say that it's placed, now check if we are wrong!
			placed = true;

			it = client_list.begin();
			for (; placed && (it != client_list.end()); ++it) {
				if ((*it) == this)
					continue; // we don't wanna take ourself into account

				increase = false;

				// Make sure clients aren't iconified or they are a desktop window
				if (!(*it)->isIconified() &&
						((*it)->getLayer() != WIN_LAYER_DESKTOP)) {

					increase = true;

					curr_x = (*it)->getFrame()->getX();
					curr_y = (*it)->getFrame()->getY();

					curr_w = (*it)->getFrame()->getWidth();
					if ((*it)->isShaded())
						curr_h = title_height + brdr_tb;
					else // not shaded
						curr_h = (*it)->getFrame()->getHeight();

					// check if we are "intruding" on some other window's place
					if (curr_x < (signed) (test_x + win_w) &&
							(curr_x + curr_w) > test_x &&
							curr_y < (signed) (test_y + win_h) &&
							(curr_y + curr_h) > test_y) {
						placed = increase = false;
						test_y = curr_y + curr_h;
					}
				}
			}

			if (placed) {
				m_x = test_x;
				m_y = test_y;
			} else if (increase) {
				test_y += step_y;
			}
		}
		test_x += step_x;
	}

	return placed;
}

//! @fn    bool placeCenteredUnderMouse(void)
//! @brief Places the client centered under the mouse
bool
Client::placeCenteredUnderMouse(void)
{
	int mouse_x, mouse_y;
	wm->getMousePosition(&mouse_x, &mouse_y);

	m_x = mouse_x - (m_width / 2);
	m_y = mouse_y - (m_height / 2);

	placeInsideScreen(); // make sure it's within the screens border

	return true;
}

//! @fn    bool placeTopLeftUnderMouse(void)
//! @brief Places the client like the menu gets placed
bool
Client::placeTopLeftUnderMouse(void)
{
	int mouse_x, mouse_y;
	wm->getMousePosition(&mouse_x, &mouse_y);

	m_x = mouse_x;
	m_y = mouse_y;

	placeInsideScreen(); // make sure it's within the screens border

	return true;

}

//! @fn    void placeInsideScreen(void)
//! @brief Makes sure the window is inside the screen.
void
Client::placeInsideScreen(void)
{
	unsigned int width, height;
	getFrameSize(width, height); // get the real size of the window

#ifdef XINERAMA
	ScreenInfo::HeadInfo head;
	wm->getScreen()->getHeadInfo(wm->getScreen()->getCurrHead(), head);

	if (m_x < head.x)
		m_x = head.x;
	else if ((m_x + width) > (head.x + head.width))
		m_x = head.x + head.width - width;

	if (m_y < head.y)
		m_y = head.y;
	else if ((m_y + height) > (head.y + head.height))
		m_y = head.y + head.height - height;
#else // !XINERAMA
	if (m_x < 0)
		m_x = 0;
	else if ((m_x + width) > wm->getScreen()->getWidth())
		m_x = wm->getScreen()->getWidth() - width;

	if (m_y < 0)
		m_y = 0;
	else if ((m_y + height) > wm->getScreen()->getHeight())
		m_y = wm->getScreen()->getHeight() - height;
#endif // XINERAMA
}

//! @fn    void getFrameSize(unsigned int &width, unsigned int &height)
//! @brief Sets width and height to the the frames size
void
Client::getFrameSize(unsigned int &width, unsigned int &height)
{
	// If we have a frame no calculations needs to be done, just
	// set the sizes to the ones the frame has
	if (m_frame) {
		width = m_frame->getWidth();
		m_height = m_frame->getHeight();
	} else {
		width = m_width;
		height = m_height;

		if (m_has_titlebar) height += calcTitleHeight();

		if (m_has_border) {
			width +=
				wm->getTheme()->getWinFocusedBorder()[BORDER_LEFT]->getWidth() +
				wm->getTheme()->getWinFocusedBorder()[BORDER_RIGHT]->getWidth();
			height +=
				wm->getTheme()->getWinFocusedBorder()[BORDER_TOP]->getHeight()+
				wm->getTheme()->getWinFocusedBorder()[BORDER_BOTTOM]->getHeight();
		}
	}
}

//! @fn    void getXClientName(void)
//! @brief Tries to get the NET_WM name, else fall back to XA_WM_NAME
void
Client::getXClientName(void)
{
	if (!wm->getExtendedWMHintString(m_window,
																	wm->getEwmhAtoms()->net_wm_name, m_name)) {

		char *name = NULL;
		if (XFetchName(dpy, m_window, &name)) {
			m_name = name;
			XFree(name);
		} else {
			m_name = "no name";
			XStoreName(dpy, m_window, "no name");
		}
	} else {
		m_has_extended_net_name = true;
	}
}

//! @fn    void getXIconName(void)
//! @brief Tries to get Client's icon name, and puts it in m_icon_name.
void
Client::getXIconName(void)
{
	char *name = NULL;
	if (XGetIconName(dpy, m_window, &name)) {
		m_icon_name = name;
		XFree(name);
	} else {
		m_icon_name = "no name";
		XSetIconName(dpy, m_window, "no name");
	}
}

//! @fn    void setWmState(unsigned long state)
//! @brief Sets the WM_STATE of the client to state
//! @param state State to set.
void
Client::setWmState(unsigned long state)
{
	unsigned long data[2];

	data[0] = state;
	data[1] = None; // No Icon

	XChangeProperty(dpy, m_window,
									wm->getIcccmAtoms()->wm_state, wm->getIcccmAtoms()->wm_state,
									32, PropModeReplace, (unsigned char *) data, 2);
}

// If we can't find a wm->wm_state we're going to have to assume
// Withdrawn. This is not exactly optimal, since we can't really
// distinguish between the case where no WM has run yet and when the
// state was explicitly removed (Clients are allowed to either set the
// atom to Withdrawn or just remove it... yuck.)
long
Client::getWmState(void)
{
	Atom real_type;
	int real_format;
	long *data, state = WithdrawnState;
	unsigned long items_read, items_left;

	int status =
		XGetWindowProperty(dpy, m_window, wm->getIcccmAtoms()->wm_state,
											 0L, 2L, False, wm->getIcccmAtoms()->wm_state,
											 &real_type, &real_format, &items_read, &items_left,
											 (unsigned char **) &data);
	if ((status  == Success) && items_read) {
		state = *data;
		XFree(data);
	}

	return state;
}

// Window gravity is a mess to explain, but we don't need to do much
// about it since we're using X borders. For NorthWest et al, the top
// left corner of the window when there is no WM needs to match up
// with the top left of our fram once we manage it, and likewise with
// SouthWest and the bottom right (these are the only values I ever
// use, but the others should be obvious.) Our titlebar is on the top
// so we only have to adjust in the first case.
void
Client::gravitate(int multiplier)
{
	// Gravity won't do shit if we don't have any decorations
	if (m_has_titlebar) {
		int gravity = (m_size->flags&PWinGravity)
			? m_size->win_gravity
			: NorthWestGravity;

		int dy;
		if (m_frame) {
			dy = m_frame->getTitleHeight();
		} else if (m_trans) {
				dy = wm->getTheme()->getWinTitleHeight() / 2;
		} else {
			dy = wm->getTheme()->getWinTitleHeight();
		}


		switch (gravity) {
		case NorthWestGravity:
	case NorthEastGravity:
	case NorthGravity:
			// do nothing, it's ok this way
			break;
		case CenterGravity:
			dy /= 2;
			break;
		default:
			dy = 0; // no gravity, set it to 0
			break;
		}

		m_y += multiplier * dy;
	}
}

//! @fn    void sendConfigureRequest(void)
//! @brief Send XConfigureEvent, letting the client know about changes
void
Client::sendConfigureRequest(void)
{
	XConfigureEvent e;

	e.type = ConfigureNotify;
	e.event = e.window = m_window;
	e.x = m_x;
	e.y = m_y;
	e.width = m_width;
	e.height = m_height;
	e.border_width = 0;
	e.above = Below;
	e.override_redirect = false;

	XSendEvent(dpy, m_window, false, StructureNotifyMask, (XEvent *) &e);
}

//! @fn void grabButton(int button, int mod, int mask, Window win, Cursor curs)
//! @brief Grabs a button on the window win
//! Grabs the button button, with the mod mod and mask mask on the window win
//! and cursor curs with "all" possible extra modifiers
void
Client::grabButton(int button, int mod, int mask, Window win, Cursor curs)
{
	unsigned int num_lock = wm->getScreen()->getNumLock();
	unsigned int scroll_lock = wm->getScreen()->getScrollLock();

	XGrabButton(dpy, button, mod,
							win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	XGrabButton(dpy, button, mod|LockMask,
							win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);

	if (num_lock) {
		XGrabButton(dpy, button, mod|num_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(dpy, button, mod|num_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
	if (scroll_lock) {
		XGrabButton(dpy, button, mod|scroll_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(dpy, button, mod|scroll_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
	if (num_lock && scroll_lock) {
		XGrabButton(dpy, button, mod|num_lock|scroll_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(dpy, button, mod|num_lock|scroll_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
}

//! @fn    void move(int x, int y)
//! @brief Update the position variables.
void
Client::move(int x, int y)
{
	m_x = x;
	m_y = y;

	sendConfigureRequest();
}

//! @fn    void resize(unsigned int w, unsigned int h)
//! @breif Resizes the client window to specified size.
void
Client::resize(unsigned int w, unsigned int h)
{
	m_width = w;
	m_height = h;
	XResizeWindow(dpy, m_window, m_width, m_height);

	sendConfigureRequest();
}

//! @fn    void stick(void)
//! @breif Toggle client sticky state
void
Client::stick(void)
{
	if (m_is_sticky)
		wm->getWorkspaces()->unstickClient(this);
	else
		wm->getWorkspaces()->stickClient(this);

	m_is_sticky = m_is_sticky ? false : true;

	setWorkspace(wm->getActiveWorkspace()); // just make sure it's visible
	updateWmStates();
}

//! @fn    void alwaysOnTop(void)
//! @breif Toggles the clients always on top state
void
Client::alwaysOnTop(void)
{
	if (m_win_layer >= WIN_LAYER_ONTOP) {
		m_win_layer = WIN_LAYER_NORMAL;
	} else {
		m_win_layer = WIN_LAYER_ONTOP;
		m_frame->raise();
	}
	updateWmStates();
}

//! @fn    void alwaysBelow(void)
//! @brief Toggles the clients always below state.
void
Client::alwaysBelow(void)
{
	if (m_win_layer <= WIN_LAYER_BELOW) {
		m_win_layer = WIN_LAYER_NORMAL;
	} else {
		m_win_layer = WIN_LAYER_BELOW;
		m_frame->lower();
	}
	updateWmStates();
}

//! @fn    void toggleBorder(void)
//! @brief Toggles the clients border.
void
Client::toggleBorder(void)
{
	if (this != m_frame->getActiveClient())
		return;

	if (m_has_border)
		m_has_border = false;
	else
		m_has_border = true;

	m_frame->setBorder();
}

//! @fn    void toggleTitlebar(void)
//! @brief Toggles the clients titlebar.
void
Client::toggleTitlebar(void)
{
	if (this != m_frame->getActiveClient())
		return;

	if (m_has_titlebar)
		m_has_titlebar = false;
	else
		m_has_titlebar = true;

	m_frame->setTitlebar();
}

//! @fn    void toggleDecor(void)
//! @brief Toggles both the clients border and titlebar.
void
Client::toggleDecor(void)
{
	if (this != m_frame->getActiveClient())
		return;

	// As we are toggeling the decor, which is both the border and titlebar
	// this could cause trouble if I looked at both before deciding which
	// to toggle, I check control the titlebar.
	if (m_has_titlebar) {
		m_has_titlebar = false;
		m_has_border = false;
	} else {
		m_has_titlebar = true;
		m_has_border = true;
	}

	m_frame->setTitlebar();
	m_frame->setBorder();
}

//! @fn    void iconify(bool notify_frame)
//! @brief Iconifies the client and adds it to the iconmenu
//! @param notify_frame Defaults to true
void
Client::iconify(void)
{
	m_is_iconified = true;
	if(!m_trans) {
#ifdef MENUS
		wm->addClientToIconMenu(this);
#endif // MENUS
		wm->findTransientsToMapOrUnmap(m_window, true);
	}
	hide();
	m_frame->hideClient(this);
}

//! @fn    void hide(bool notify_frame)
//! @brief Sets the client to WithdrawnState and hides it.
void
Client::hide(void)
{
	if (m_is_hidden)
		return;
	m_is_hidden = true;

	if (m_has_focus)	{ // we cant have focus if we're hidden
		m_has_focus = false;
		wm->findClientAndFocus(None);
	}

	filteredUnmap(); // unmap the window
	if (m_is_iconified) // set the state of the window
		setWmState(IconicState);
	else
		setWmState(WithdrawnState);
	updateWmStates();
}

//! @fn    void unhide(bool notify_frame)
//! @brief Unhides the client.
void
Client::unhide(void)
{
	if (!m_is_hidden)
		return;
	m_is_hidden = false;

	if(m_is_iconified) { // remove ourself from the iconmenu
		m_is_iconified = false;
#ifdef MENUS
		wm->removeClientFromIconMenu(this);
#endif // MENUS
	}

	XMapWindow(dpy, m_window); // unhide the window
	setWmState(NormalState); // update the state and wm hints
	updateWmStates();

	if(!m_trans) // unmap our transient windows if we have any
		wm->findTransientsToMapOrUnmap(m_window, false);
}

//! @fn    void close(void)
//! @brief Sends an WM_DELETE message to the client, else kills it.
void
Client::close(void)
{
	int count;
	bool found = false;
	Atom *protocols;

	if (XGetWMProtocols(dpy, m_window, &protocols, &count)) {
		for (int i = 0; i < count; ++i) {
			if (protocols[i] == wm->getIcccmAtoms()->wm_delete) {
				found = true;
				break;
			}
		}

		XFree(protocols);
	}

	if (found)
		sendXMessage(m_window, wm->getIcccmAtoms()->wm_protos,
								 NoEventMask, wm->getIcccmAtoms()->wm_delete);
	else
		kill();
}

//! @fn    void kill(void)
//! @breif Kills the client using XKillClient
void
Client::kill(void)
{
	XKillClient(dpy, m_window);
}

//! @fn    void setFocus(bool focus)
//! @brief Redraws the clients titlebar, if we are active.
void
Client::setFocus(bool focus)
{
	m_has_focus = focus;

	if (this == m_frame->getActiveClient())
		m_frame->setFocus(focus);
}

//! @fn    bool setPUPosition(void)
//! @brief Sets the clients position based on P or U position.
//! @return Returns true on success, else false.
bool
Client::setPUPosition(void)
{
	if ((m_size->flags&PPosition) ||
			(m_size->flags&USPosition)) {

		if (!m_x)
			m_x = m_size->x;
		if (!m_y)
			m_y = m_size->y;

		return true;
	}
	return false;
}

//! @fn    bool getIncSize(unsigned int *r_w, unsigned int *r_h, unsigned int w, unsigned int h)
//! @brief Get the size closest to the ResizeInc incremeter
//! @param r_w Pointer to put the new width in
//! @param r_h Pointer to put the new height in
//! @param w Width to calculate from
//! @param h Height to calculate from
bool
Client::getIncSize(unsigned int *r_w, unsigned int *r_h,
									 unsigned int w, unsigned int h)
{
	int basex, basey;

	if (m_size->flags&PResizeInc) {
		basex = (m_size->flags&PBaseSize)
			? m_size->base_width
			: (m_size->flags&PMinSize) ? m_size->min_width : 0;

		basey = (m_size->flags&PBaseSize)
			? m_size->base_height
			: (m_size->flags&PMinSize) ? m_size->min_height : 0;

		*r_w = w - ((w - basex) % m_size->width_inc);
		*r_h = h - ((h - basey) % m_size->height_inc);

		return true;
	}

	return false;
}

//! @fn    MwmHints* getMwmHints(Window win)
//! @brief Gets a MwmHint structure from a window. Doesn't free memory.
MwmHints*
Client::getMwmHints(Window win)
{
	Atom real_type; int real_format;
	unsigned long items_read, items_left;
	MwmHints *data;

	int status =
		XGetWindowProperty(dpy, win, wm->getMwmHintsAtom(),
											 0L, 20L, False, wm->getMwmHintsAtom(),
											 &real_type, &real_format,
											 &items_read, &items_left,
											 (unsigned char **) &data);

	if ((status == Success) && (items_read >= MWM_HINT_ELEMENTS))
		return data;
	else
		return NULL;
}

//! @fn    void handleConfigureRequest(XConfigureRequestEvent *e)
//! @brief Handle XConfgiureRequestEvents
void
Client::handleConfigureRequest(XConfigureRequestEvent *e)
{
	// We only do size/position and stacking changes if we
	// are the active client. Also look and se if it's for us.
	if ((!m_frame) || (this != m_frame->getActiveClient()))
		return;

#ifdef DEBUG
	cerr << __FILE__ << "@" << __LINE__ << ": "
			 << "handleConfigureRequest(" << e->window << ")" << endl
			 << "x: " << e->x << " y: " << e->y << endl
			 << "width: " << e->width << " height: " << e->height << endl
			 << "above: " << e->above << endl
			 << "detail: " << e->detail << endl << endl;
#endif // DEBUG

	// If the size has changes we're going to update both
	// the frame and the client window BUT only if we are the active client
	if ((e->value_mask&CWX) || (e->value_mask&CWY) ||
			(e->value_mask&CWWidth) || (e->value_mask&CWHeight)) {

		int x = m_frame->getX();
		int y = m_frame->getY();
		unsigned int width = m_frame->getWidth();
		unsigned int  height = m_frame->getHeight();

		// Uppdate the geometry so that it matches the frame
		if (e->value_mask&CWX)
			x = e->x - m_frame->borderLeft();
		if (e->value_mask&CWY)
			y = e->y - m_frame->borderTop() - m_frame->getTitleHeight();
		if (e->value_mask&CWWidth)
			width = e->width + m_frame->borderLeft() + m_frame->borderRight();
		if (e->value_mask&CWHeight)
			height = e->height + m_frame->borderTop() + m_frame->borderBottom() +
				m_frame->getTitleHeight();

		m_frame->updateFrameGeometry(x, y, width, height);
		m_frame->updateClientGeometry();

		m_is_shaded = false; // can't be shaded after the resize

#ifdef SHAPE
		if (e->value_mask&(CWWidth|CWHeight)) {
			m_frame->setShape();
		}
#endif
	}

	// Update the stacking mode
	if (e->value_mask&CWStackMode) {
		if (e->value_mask&CWSibling) {
			switch(e->detail) {
			case Above:
				wm->getWorkspaces()->stackClientAbove(this, e->above, m_on_workspace);
				break;
			case Below:
				wm->getWorkspaces()->stackClientBelow(this, e->above, m_on_workspace);
				break;
			case TopIf:
			case BottomIf:
				// TO-DO: What does occlude mean?
				break;
			}
		} else {
			switch(e->detail) {
			case Above:
				m_frame->raise();
				break;
			case Below:
				m_frame->lower();
				break;
			case TopIf:
			case BottomIf:
				// TO-DO: Why does the manual say that it should care about siblings
				// even if we don't have any specified?
				break;
			}
		}
	}
}

//! @fn    void handleMapRequest(XMapRequestEvent *e)
//! @brief Handles map request
void
Client::handleMapRequest(XMapRequestEvent *e)
{
#ifdef DEBUG
	cerr << __FILE__ << "@" << __LINE__ << ": "
			 << "handleMapRequest(" << this << ")" << endl;
#endif // DEBUG

	if (!m_is_sticky && (m_on_workspace != signed(wm->getActiveWorkspace()))) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Ignoring MapRequest, not on current workspace!" << endl;
#endif // DEBUG
		return;
	}

	if (m_frame)
		m_frame->unhideClient(this);
	else
		unhide();
}

//! @fn    void handleUnmapEvent(XUnmapEvent *e)
//! @brief
//! @param e XUnmapEvent to handle, this isn't used though
//! @bug When the client (only transients it seems) won't unmap if they aren't on the same workspace as the active one.
void
Client::handleUnmapEvent(XUnmapEvent *e)
{
	if (e->window != e->event)
		return;

#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Unmapping client: " << this << endl;
#endif // DEBUG

	//	m_is_alive = false; // TO-DO: How to make this work?
	delete this;
}

// This happens when a window is iconified and destroys itself. An
// Unmap event wouldn't happen in that case because the window is
// already unmapped.
void
Client::handleDestroyEvent(XDestroyWindowEvent *e)
{
	m_is_alive = false;
	delete this;
}

// If a client wants to iconify itself (boo! hiss!) it must send a
// special kind of ClientMessage. We might set up other handlers here
// but there's nothing else required by the ICCCM.
void
Client::handleClientMessage(XClientMessageEvent *e)
{
	EwmhAtoms *ewmh = wm->getEwmhAtoms(); // convinience

	bool state_remove = false;
	bool state_add = false;
	bool state_toggle = false;

	if (e->message_type == ewmh->net_wm_state) {
		if(e->data.l[0]== NET_WM_STATE_REMOVE) state_remove = true;
		else if(e->data.l[0]== NET_WM_STATE_ADD) state_add = true;
		else if(e->data.l[0]== NET_WM_STATE_TOGGLE)	state_toggle = true;

		// There is no modal support in pekwm yet
//		if ((e->data.l[1] == (long) ewmh->net_wm_state_modal) ||
//			(e->data.l[2] == (long) ewmh->net_wm_state_modal)) {
//			is_modal=true;
//		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_sticky) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_sticky)) {
			// Well, I'm setting the desktop because it updates the desktop state
			// because if we are sticky we appear on all. Also, if we unstick
			// we should be on the current desktop
			if(state_add && !m_is_sticky) stick();
			else if(state_remove && m_is_sticky) stick();
			else if(state_toggle) stick();

			setWorkspace(wm->getActiveWorkspace());
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_maximized_vert) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_maximized_vert)) {

			if (state_add) m_is_maximized_vertical = false;
			else if (state_remove) m_is_maximized_vertical = true;

			m_frame->maximizeVertical();
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_maximized_horz) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_maximized_horz)) {
			if (state_add) m_is_maximized_horizontal = false;
			else if (state_remove) m_is_maximized_horizontal = true;

			m_frame->maximizeHorizontal();
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_shaded) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_shaded)) {
			if (state_add) m_is_shaded = false;
			else if (state_remove) m_is_shaded = true;

			m_frame->shade();
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_skip_taskbar) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_skip_taskbar)) {
		if (state_add) m_skip_taskbar = true;
			else if (state_remove) m_skip_taskbar = false;
			else if (state_toggle) m_skip_taskbar = (m_skip_taskbar) ? true : false;
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_skip_pager) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_skip_pager)) {
			if (state_add) m_skip_pager=true;
			else if (state_remove) m_skip_pager=false;
			else if (state_toggle) m_skip_pager = (m_skip_pager) ? true : false;
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_hidden) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_hidden)) {
			if (state_add && !m_is_iconified) iconify();
			else if (state_remove && m_is_iconified) m_frame->unhideClient(this);
			else if (state_toggle) {
				if (m_is_iconified) {
					m_frame->unhideClient(this);
				} else {
					iconify();
				}
			}
		}

		if ((e->data.l[1] == (long) ewmh->net_wm_state_fullscreen) ||
				(e->data.l[2] == (long) ewmh->net_wm_state_fullscreen)) {
			// TO-DO: Add support for toggling net_wm_state_fullscreen
		}

		updateWmStates();
	} else if (e->message_type == ewmh->net_active_window) {
		if (this != m_frame->getActiveClient()) {
			m_frame->activateClient(this);
		} else if (m_is_hidden) {
			m_frame->unhideClient(this);
		} else {
			giveInputFocus();
		}
	} else if (e->message_type == ewmh->net_close_window) {
		kill();
	} else if (e->message_type == ewmh->net_wm_desktop) {
		setWorkspace(e->data.l[0]);
	} else if (e->message_type == wm->getIcccmAtoms()->wm_change_state &&
						 e->format == 32 && e->data.l[0] == IconicState)  {
		iconify();
	}
}

void
Client::handlePropertyChange(XPropertyEvent *e)
{
	EwmhAtoms *ewmh = wm->getEwmhAtoms(); // convinience

	if (e->atom == ewmh->net_wm_desktop) {
		// not calling set desktop because that will end up in an infinite loop
		unsigned int workspace = wm->findDesktopHint(m_window);
		if (workspace < wm->getWorkspaces()->getNumWorkspaces()) {
			if(m_on_workspace != (signed) wm->getActiveWorkspace()) {
				m_frame->hideClient(this);
			}
		}
	} else if (e->atom == ewmh->net_wm_strut) {
		// Do we want to update our strut?
		int num=0;
		CARD32 *temp_strut = NULL;
		temp_strut = (CARD32*) wm->getExtendedNetPropertyData(
			m_window, ewmh->net_wm_strut, XA_CARDINAL, &num);

		if(m_has_strut) {
			wm->removeStrut(m_client_strut);

			delete m_client_strut;
			m_client_strut = NULL;
			m_has_strut = false;
		}

		if(temp_strut) {
			if (! m_client_strut)
				m_client_strut = new Strut;

			m_client_strut->west = temp_strut[0];
			m_client_strut->east = temp_strut[1];
			m_client_strut->north = temp_strut[2];
			m_client_strut->south = temp_strut[3];

			wm->addStrut(m_client_strut);

			m_has_strut = true;

			XFree(temp_strut);
		}
	}

	if(m_has_extended_net_name) {
		if(e->atom == ewmh->net_wm_name) {
			getXClientName();

			// TO-DO: UTF8 support so we can handle the hint
			m_frame->repaint(m_has_focus);
		}
	}

	long dummy;
	switch (e->atom)  {
		case XA_WM_NAME:
			if(!m_has_extended_net_name) {
				getXClientName();

				m_frame->repaint(m_has_focus);
			}
			break;

		case XA_WM_NORMAL_HINTS:
			XGetWMNormalHints(dpy, m_window, m_size, &dummy);
			break;

		case XA_WM_TRANSIENT_FOR:
			if(! m_trans) {
				XGetTransientForHint(dpy, m_window, &m_trans);
				// now if we have become an transient window,
				// we should update the title height
				m_frame->updateTitleHeight();
				m_frame->hideAllButtons();
			}
		break;
	}
}

void
Client::handleColormapChange(XColormapEvent *e)
{
	if (e->c_new) {
		m_cmap = e->colormap;
		XInstallColormap(dpy, m_cmap);
	}
}

#ifdef SHAPE
void
Client::handleShapeChange(XShapeEvent *e)
{
	if (m_frame->getActiveClient())
		m_frame->setShape();
}
#endif

int
Client::sendXMessage(Window window, Atom atom, long mask, long value)
{
	XEvent e;

	e.type = e.xclient.type = ClientMessage;
	e.xclient.display = dpy;
	e.xclient.message_type = atom;
	e.xclient.window = window;
	e.xclient.format = 32;
	e.xclient.data.l[0] = value;
	e.xclient.data.l[1] = CurrentTime;
	e.xclient.data.l[2] = e.xclient.data.l[3] = e.xclient.data.l[4] = 0l;

	return XSendEvent(dpy, window, false, mask, &e);
}

void
Client::setWorkspace(unsigned int workspace)
{
	if (workspace >= wm->getWorkspaces()->getNumWorkspaces())
		return;

	if (m_on_workspace != signed(workspace)) {
		if (this == wm->getFocusedClient())
			wm->findClientAndFocus(None);

		wm->getWorkspaces()->removeClient(this, m_on_workspace);
		m_on_workspace = workspace;
		wm->getWorkspaces()->addClient(this, m_on_workspace);

		m_frame->unhideClient(this);
	}

	// set the desktop we're on, if we are sticky we are on _all_ desktops
	if (m_is_sticky) {
		wm->setExtendedWMHint(m_window, wm->getEwmhAtoms()->net_wm_desktop,
													NET_WM_STICKY_WINDOW);
	} else {
		wm->setExtendedWMHint(m_window, wm->getEwmhAtoms()->net_wm_desktop,
													m_on_workspace);
	}

	loadAutoProps(AutoProps::APPLY_ON_WORKSPACE_CHANGE);
}

void
Client::updateWmStates(void)
{
	// Clients will always have a false modal property set
	// since there is no support for modal windows in pekwm.

	wm->setExtendedNetWMState(m_window,
														false, // modal
														m_is_sticky,
														m_is_maximized_vertical,
														m_is_maximized_horizontal,
														m_is_shaded,
														m_skip_taskbar,
														m_skip_pager,
														m_is_iconified,
														false); // TO-DO: Add support for net_wm_fullscreen
}

//! @fn    void filteredUnmap(void)
//! @brief Unmaps the client withouth causing unmap events
void
Client::filteredUnmap(void)
{
	XSelectInput(dpy, m_window, NoEventMask);
	XUnmapWindow(dpy, m_window);
	XSelectInput(dpy, m_window,
							 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @fn    void filteredReparent(Window parent, int x, int y);
//! @brief Reparents the client withouth causing unmap events
//! @param parent Window to reparent the client to
//! @param x X offset
//! @param y Y offset
void
Client::filteredReparent(Window parent, int x, int y)
{
	XSelectInput(dpy, m_window, NoEventMask);
	XReparentWindow(dpy, m_window, parent, x, y);
	XSelectInput(dpy, m_window,
							 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}
