//
// frame.cc for pekwm
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

#include "frame.hh"
#include "config.hh"

#include <algorithm>
#include <functional>
#include <cstdio> // for snprintf

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::vector;
using std::list;
using std::mem_fun;

Frame::Frame(WindowManager *w, Client *cl) :
wm(w),
m_client(NULL), m_button(NULL),
m_frame(None), m_title(None),
m_title_pixmap(None),
m_x(0), m_y(0), m_width(1), m_height(1),
m_title_height(0),
m_pixmap_width(0), m_pixmap_height(0),
m_button_width_left(0), m_button_width_right(0),
m_old_x(0), m_old_y(0), m_old_width(1), m_old_height(1),
m_pointer_x(0), m_pointer_y(0),
m_old_cx(0), m_old_cy(0),
m_is_hidden(false)
{
	// setup basic pointers
	scr = wm->getScreen();
	theme = wm->getTheme();

	// initialize array values
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i)
		m_border[i] = None;
	for (unsigned int i = 0; i < 5; ++i)
		m_last_button_time[i] = 0;

	// this will construct the frame hosting the clients, based on m_client
	// geometry and props
	constructFrame(cl);

	// now insert the client in the frame we created, and we do not
	// give it focus.
	insertClient(cl, false);

	// set the window states, shaded, maiximized...
	initFrameState();

	// make sure we don't cover anything we shouldn't
	if (!m_client->hasStrut())
		fixGeometryBasedOnStrut(wm->getMasterStrut());

	wm->addToFrameList(this);
}

Frame::~Frame()
{
	// if we have any client left in the frame, lets make new frames for them
	if (m_client_list.size()) {
		vector<Client*>::iterator it = m_client_list.begin();
		for (; it != m_client_list.end(); ++it) {
			Frame *frame = new Frame(wm, *it);
			(*it)->setFrame(frame);
		}
	}
	m_client_list.clear();

	unloadButtons();

	// free the frame windows
	XUngrabButton(scr->getDisplay(), AnyButton, AnyModifier, m_title);
	XDestroyWindow(scr->getDisplay(), m_title);

	// free border windows
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		XDestroyWindow(scr->getDisplay(), m_border[i]);
	}

	XDestroyWindow(scr->getDisplay(), m_frame);

	// free the titlebar pixmap
	if (m_title_pixmap != None)
		XFreePixmap(scr->getDisplay(), m_title_pixmap);

	wm->removeFromFrameList(this);
}

//! @fn    void constructFrame(Client *client)
//! @brief Creates the frame window and the title, also initialises it's values
//! @param client Client to base initial values on
void
Frame::constructFrame(Client *client)
{
	Display *dpy = scr->getDisplay(); // convinience

	XGrabServer(dpy);

	m_title_height = client->calcTitleHeight();

	// frame position and geometry
	m_x = client->getX();
	m_y = client->getY() - m_title_height;
	m_width = client->getWidth();
	m_height = client->getHeight();
	if (client->hasBorder()) {
		m_width += theme->getWinUnfocusedBorder()[BORDER_LEFT]->getWidth() +
			theme->getWinUnfocusedBorder()[BORDER_RIGHT]->getWidth();
		m_height += theme->getWinUnfocusedBorder()[BORDER_TOP]->getHeight() +
			theme->getWinUnfocusedBorder()[BORDER_BOTTOM]->getHeight();
	}
	m_height += client->hasTitlebar() ? m_title_height : 0;

	XSetWindowAttributes pattr;
	pattr.do_not_propagate_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
	pattr.override_redirect = false;
	pattr.event_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		EnterWindowMask|
		SubstructureRedirectMask|SubstructureNotifyMask;

	// Create frame window
	m_frame =
		XCreateWindow(dpy, scr->getRoot(),
									m_x, m_y,	m_width, m_height, 0,
									scr->getDepth(), CopyFromParent, CopyFromParent,
									CWOverrideRedirect|CWDontPropagate|CWEventMask,
									&pattr);

	// Create the titlebar
	// The title height _might_ be 0 so make sure it atleast get 1, otherwise
	// it won't be a valid size == the window won't be created
	m_title =
		XCreateWindow(dpy, m_frame,
									-1, -1, m_width, 1, 0,
									scr->getDepth(), CopyFromParent, CopyFromParent,
									CWOverrideRedirect|CWDontPropagate|CWEventMask,
									&pattr);

	// create a border for the frame
	createBorderWindows();

	// add buttons to the frame
	loadButtons();

	XMapSubwindows(dpy, m_frame);
	// now show the window
	if (!client->isIconified()) {
		m_is_hidden = false;

		XMapWindow(dpy, m_frame);
	} else {
		m_is_hidden = true;
	}

	// Well, by grabbing these I can Reply client window button presses
	// so that you can use button3 for lowering the window but you can
	// still use button3 in the application
	XGrabButton(dpy, Button1, AnyModifier, m_frame,
							True, ButtonPressMask|ButtonReleaseMask,
							GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button2, AnyModifier, m_frame,
							True, ButtonPressMask|ButtonReleaseMask,
							GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button3, AnyModifier, m_frame,
							True, ButtonPressMask|ButtonReleaseMask,
							GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button4, AnyModifier, m_frame,
							True, ButtonPressMask|ButtonReleaseMask,
							GrabModeSync, GrabModeAsync, None, None);
	XGrabButton(dpy, Button5, AnyModifier, m_frame,
							True, ButtonPressMask|ButtonReleaseMask,
							GrabModeSync, GrabModeAsync, None, None);

	XSync(dpy, false);
	XUngrabServer(dpy);
}

//! @fn    void createBorderWindows(void)
//! @brief Create windows used for holding the border pixmaps
void
Frame::createBorderWindows(void)
{
	XSetWindowAttributes attr;
	attr.event_mask = ButtonMotionMask;
	attr.cursor = None; // TO-DO: Default was named?

	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		attr.cursor = wm->getCursors()[i];

		m_border[i] =
			XCreateWindow(scr->getDisplay(), m_frame,
										-1, -1, 1, 1, 0,
										scr->getDepth(), InputOutput, CopyFromParent,
										CWEventMask|CWCursor, &attr);
	}
}

//! @fn    void loadTheme(void)
//! @brief Loads and if allready loaded, it unloads the necessary parts
void
Frame::loadTheme(void)
{
	// First we uppdate the title and make sure it's (in)visible.
	m_title_height = m_client->calcTitleHeight();
	if (m_title_height)
		XRaiseWindow(scr->getDisplay(), m_title);
	else
		XLowerWindow(scr->getDisplay(), m_title);

	// Then we uppdate the dimensions of the frame
	m_width = m_client->getWidth() + borderLeft() + borderRight();
	m_height = m_client->getHeight() + m_title_height +
		borderTop() + borderBottom();
	// move all the clients so that they don't sit on top or, under the titlebar
	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		XMoveWindow(scr->getDisplay(), (*it)->getWindow(),
								borderLeft(), (*it)->calcTitleHeight() + borderTop());
	}

	// make sure the top border is (in)visible
	if (!borderTop()) {
		XLowerWindow(scr->getDisplay(), m_border[BORDER_TOP]);
		XLowerWindow(scr->getDisplay(), m_border[BORDER_TOP_LEFT]);
		XLowerWindow(scr->getDisplay(), m_border[BORDER_TOP_RIGHT]);
	} else {
		XRaiseWindow(scr->getDisplay(), m_border[BORDER_TOP]);
		XRaiseWindow(scr->getDisplay(), m_border[BORDER_TOP_LEFT]);
		XRaiseWindow(scr->getDisplay(), m_border[BORDER_TOP_RIGHT]);
	}

	updateFrameSize();
	rearangeBorderWindows();

	loadButtons(); // unloads if allready have any buttons loaded
	updateButtonPosition();
	setFocus(m_client->hasFocus());
}


//! @fn    void initFrameStat(void)
void
Frame::initFrameState(void)
{
	if (m_client->isShaded()) {
		m_client->m_is_shaded = false;
		shade();
	}
	if (m_client->isMaximizedVertical()) {
		m_client->m_is_maximized_vertical = false;
		maximizeVertical();
	}
	if (m_client->isMaximizedHorizontal()) {
		m_client->m_is_maximized_horizontal = false;
		maximizeHorizontal();
	}

	if (m_client->isIconified())
		m_client->iconify();
	// I'm doing this so that it'll report itself beeing on all desktops
	if (m_client->isSticky())
		m_client->setWorkspace(wm->getActiveWorkspace());
}

//! @fn    void activateNextClient(void)
//! @brief Makes the next client in the frame active (wraps)
void
Frame::activateNextClient(void)
{
	if (m_client_list.size() < 2)
		return; // no client to switch to

	// find the active clients position
	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), m_client);

	Client *new_client;
	if (++it < m_client_list.end()) {
		new_client = *it;
	} else {
		new_client = m_client_list.front();
	}

	activateClient(new_client);
}

//! @fn    void activatePrevClient(void)
//! @brief Makes the prev client in the frame active (wraps)
void
Frame::activatePrevClient(void)
{
	if (m_client_list.size() < 2)
		return; // no client to switch to

	// find the active clients position
	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), m_client);

	Client *new_client;

	if (--it >= m_client_list.begin()) {
		new_client = *it;
	} else {
		new_client = m_client_list.back();
	}

	activateClient(new_client);
}

//! @fn    void insertClient(Client *c, bool focus)
//! @brief Insert a client to the frame and sets it active
//! @param c Client to insert.
//! @param focus Defaults to true.
void
Frame::insertClient(Client *client, bool focus)
{
	if (!client)
		return;

	m_client_list.push_back(client);

	client->setFrame(this);

	if (!m_client) {
		m_client = client; // needed for border macros
		client->filteredReparent(m_frame,
														 borderLeft(), borderTop() + m_title_height);
		m_client = NULL;
	} else {
		client->filteredReparent(m_frame,
														 borderLeft(), borderTop() + m_title_height);
	}

	activateClient(client, focus);
}

//! @fn    void removeClient(Client *client)
//! @brief Removes the Client client from the frame
//! Removes the client client, and if the frame becomes empty
//! it'll delete itself otherwise it'll set the first client to active
//!
//! @param client Client to remove
void
Frame::removeClient(Client *client)
{
	if (!client)
		return;

	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), client);

	if (it != m_client_list.end()) {
		m_client_list.erase(it);

		client->setFrame(NULL);

		// Only move the client to the root if it's alive ( it's running )
		if (client->isAlive()) {
			client->filteredReparent(scr->getRoot(), client->getX(), client->getY());
		}

		// if we don't hold anymore clients, then remove the frame
		if (!m_client_list.size()) {
			delete this;
		} else if (m_client == client) {
			if (allClientsAreHidden()) {
				m_client = m_client_list.front();
				hide();
			} else {
				activateClient(m_client_list.front(), false);
				setFocus(m_client->hasFocus());
			}
		}
	}
}

//! @fn    bool clientIsInFrame(Client *c)
//! @brief Checks if Client c is contained in this Frame
//! @return true if the Client c is in the frame else false
bool
Frame::clientIsInFrame(Client *c)
{
	if (!c)
		return false;

	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), c);

	if (it != m_client_list.end())
		return true;
	return false;
}

//! @fn    bool allClientsAreHidden(void)
//! @return true if all clients are hidden, else false
bool
Frame::allClientsAreHidden(void)
{
	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		if (!(*it)->isHidden()) {
			return false;
		}
	}
	return true;
}

//! @fn    bool allClientsAreOfSameClass(void)
//! @return true if all clients are of same class, else false
bool
Frame::allClientsAreOfSameClass(void)
{
 // we don't have more than one client, then all must be the same
	if (m_client_list.size() < 2)
		return true;

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		if ((*it) == m_client)
			continue; // skip the active client

		if (m_client->getClassHint() != (*it)->getClassHint()) {
			return false;
		}
	}
	return true;
}

//! @fn    void activateClient(Client *client)
//! @brief Activates the Client m_client
//! @param client Client to activate.
//! @param focus Defaults to true
void
Frame::activateClient(Client *client, bool focus)
{
	if (!client || !clientIsInFrame(client) || (client == m_client))
		return; // no client to activate

	Client *old_client = m_client;
	m_client = client;

	// set the new clients properties based on the old
	if (old_client) {
		// If we change from a Transient to Normal we need to uppdate
		// the titlebar height and show/hide titlebar buttons
		if ((client->getTransientWindow() ? true : false) !=
				(old_client->getTransientWindow() ? true : false)) {
			updateTitleHeight();

			if (client->getTransientWindow()) {
				hideAllButtons();
			} else {
				updateButtonPosition();
			}
		}

		if (client->isHidden())
			client->unhide(); // make sure the client is visible
		if (client->isShaded() != old_client->isShaded())
			client->m_is_shaded = old_client->isShaded();	// set correct shade state

		// fix the stacking of the client, but don't restack any X windows
		wm->getWorkspaces()->stackClientAbove(client, old_client->getWindow(),
																					client->onWorkspace(), false);
	} else {
		updateFrameSize();
		if (m_client->hasBorder())
			setBorderFocus(m_client->hasFocus());
	}

	updateClientGeometry(); // fit the new client to the frame
#ifdef SHAPE
	if (wm->hasShape()) {
		setShape(); // reshape the frame.
	}
#endif // SHAPE

	XRaiseWindow(scr->getDisplay(), client->getWindow());

	// Make sure that the bottom border are ontop of the client
	if (client->hasBorder()) {
		for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
			XRaiseWindow(scr->getDisplay(), m_border[i]);
		}
	}

	if (focus)
		m_client->giveInputFocus();

	// One might think I should do a repaint here, _but_ I don't need to,
	// I'll get the repaint done when the new client gets focused
}

void
Frame::activateClientFromPos(int x)
{
	Client *client = getClientFromPos(x);
	if (client && (client != m_client))
		activateClient(client);
}

//! @fn    void activateClientFromNum(unsigned int n)
//! @brief Activates the client numbered n in the frame.
//! @param n Client number to activate.
void
Frame::activateClientFromNum(unsigned int n)
{
	if (n >= m_client_list.size())
		return;

	// Only activate if it's another client
	if (m_client_list[n] != m_client)
		activateClient(m_client_list[n]);
}

//! @fn    void moveClientNext(void)
//! @brief Moves the active one step to the right in the frame
//! Moves the active one step to the right in the frame, this is used
//! to get more structure in frames with alot clients grouped
void
Frame::moveClientNext(void)
{
	if (m_client_list.size() < 2)
		return;

	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), m_client);

	if (it != m_client_list.end()) {
		vector<Client*>::iterator n_it = it;
		if (++n_it < m_client_list.end()) {
			*it = *n_it; // put the next client at the active clients place
			*n_it = m_client; // move the active client to the next client

			// changed the order of the client, lets redraw the titlebar
			repaint(m_client->hasFocus());
		}
	}
}

//! @fn    void moveClientPrev(void)
//! @brief Moves the active one step to the left in the frame
//! Moves the active one step to the left in the frame, this is used
//! to get more structure in frames with alot clients grouped
void
Frame::moveClientPrev(void)
{
	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), m_client);

	if (it != m_client_list.end()) {
		vector<Client*>::iterator n_it = it;
		if (it > m_client_list.begin()) {
			*it = *(--n_it); // move previous client to the active client
			*n_it = m_client; // move active client back

			// changed the order of the client, lets redraw the titlebar
			repaint(m_client->hasFocus());
		}
	}
}

//! @fn    FrameButton* findButton(Window win)
//! @brief Tries to find the corresponding button from the win
//! @param win Window to use as criteria when searching
//! @return Returns a pointer to the FrameButton if it's found, else NULL
FrameButton*
Frame::findButton(Window win)
{
	if (!win)
		return NULL;

	list<FrameButton *>::iterator it = m_button_list.begin();
	for (; it != m_button_list.end(); ++it) {
		if ((*it)->getWindow() == win) {
			return *it;
		}
	}

	return NULL;
}


#ifdef SHAPE
void
Frame::setShape(void)
{
	int n, order;
	XRectangle temp, *dummy;

	dummy = XShapeGetRectangles(scr->getDisplay(), m_client->getWindow(),
															ShapeBounding, &n, &order);

	if (n > 1) {
		XShapeCombineShape(scr->getDisplay(), m_frame, ShapeBounding,
											 0, m_title_height, m_client->getWindow(),
											 ShapeBounding, ShapeSet);

		temp.x = -borderLeft();
		temp.y = -borderTop();
		temp.width = m_width;
		temp.height = m_title_height + borderTop();

		XShapeCombineRectangles(scr->getDisplay(), m_frame, ShapeBounding,
														0, 0, &temp, 1, ShapeUnion, YXBanded);

		temp.x = 0;
		temp.y = 0;
		temp.width = m_client->getWidth();
		temp.height = m_title_height - borderTop();

		XShapeCombineRectangles(scr->getDisplay(), m_frame, ShapeClip,
														0, m_title_height, &temp, 1, ShapeUnion, YXBanded);

		m_client->setShaped(true);

	} else if (m_client->hasBeenShaped()) {
		temp.x = -borderLeft();
		temp.y = -borderTop();
		temp.width = m_width;
		temp.height = m_height;

		XShapeCombineRectangles(scr->getDisplay(), m_frame, ShapeBounding,
														0, 0, &temp, 1, ShapeSet, YXBanded);
	}

	XFree(dummy);
}
#endif // SHAPE


//! @fn    Client* getClientFromPos(unsigned int x)
//! @brief Searches the clien positioned at position x of the frame title
//! @param x Where in the title to look
//! @return Pointer to the Client found, if no Client found it returns NULL
Client*
Frame::getClientFromPos(unsigned int x)
{
	if ((x > m_width) || !m_client_list.size())
		return NULL;

	unsigned int pos, ew;
	// we don't have any buttons on transient windows
	if (m_client->getTransientWindow()) {
		pos = 0;
		ew = m_width / m_client_list.size();
	} else {
		pos = m_button_width_left;
		ew = (m_width - pos - m_button_width_right) / m_client_list.size();
	}

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it, pos +=ew) {
		if ((x >= pos) && (x <= (pos + ew))) {
			return *it;
		}
	}

	return NULL;
}

//! @fn    void repaint(void)
//! @brief Redraws the Frame's title.
void
Frame::repaint(bool focus)
{
	if (!m_client || !m_client->hasTitlebar() || !m_client_list.size() ||
			!theme->getWinTitleHeight())
		return;

#ifdef DEBUG
	//	cerr << __FILE__ << "@" << __LINE__ << ": "<< "repaint: " << this
	//			 << " with active client: " << m_client << endl;
#endif // DEBUG

	Display *dpy = scr->getDisplay(); //convinience
	unsigned int num = m_client_list.size(); // convinience

	// TO-DO: Set the max values to something sensible
	unsigned int width = (m_width > 4096) ? 4096 : m_width;

	// see if we can reuse the old pixmap.
	// TO-DO: Should we more memory conservative here? Only reusing if
	// the size is the same == shrinking the pixmaps?
	if (m_title_pixmap != None) {
		if ((width > m_pixmap_width) || (m_title_height > m_pixmap_height)) {
			XFreePixmap(dpy, m_title_pixmap);
			m_title_pixmap = None;
		}
	}

	// create new titlebar pixmap to draw to.
	if (m_title_pixmap == None) {
		m_title_pixmap =
			XCreatePixmap(dpy, scr->getRoot(), width, m_title_height, scr->getDepth());
		m_pixmap_width = width;
		m_pixmap_height = m_title_height;
	}

	// draw the base pixmap on the titlebar
	if (focus) {
		theme->getWinFocusedPixmap()->
			draw(m_title_pixmap, 0, 0, width, m_title_height);
	}	else {
		theme->getWinUnfocusedPixmap()->
			draw(m_title_pixmap, 0, 0, width, m_title_height);
	}

	PekFont *font = theme->getWinFont();
	// set the correct color of the title font
	if (!focus) {
		font->setColor(theme->getWinUnfocusedText());
	} else if (num > 1) {
		font->setColor(theme->getWinFocusedText());
	} else {
		font->setColor(theme->getWinSelectedText());
	}

	// convinience
	Image *img_separator = focus
		? theme->getWinFocusedSeparator()
		: theme->getWinUnfocusedSeparator();

	unsigned int x = 0;
	unsigned int ew = m_width - borderLeft() - borderRight();
	// we don't have any buttons on transient windows
	if (!m_client->getTransientWindow()) {
		x += m_button_width_left;
		ew -= m_button_width_left + m_button_width_right;
	}
	ew = (ew - ((num - 1) * img_separator->getWidth())) / num;

	vector<Client*>::iterator it = m_client_list.begin();
	for (int i = 0; it != m_client_list.end(); ++i, ++it, x += ew) {
		if (num > 1) {
			if (i > 0) { // draw separator
				img_separator->draw(m_title_pixmap, x, 0);
				x += img_separator->getWidth() + 1;
			}

			// The active client has another color
			if (focus && (*it == m_client)) {
				theme->getWinSelectedPixmap()->draw(m_title_pixmap,
																						x, 0, ew, m_title_height);
				font->setColor(theme->getWinSelectedText());
			}
		}

		// if the active client is a transient one, the title will be too small
		// to show any names, therefore we don't draw any
		if (!m_client->getTransientWindow()) {
			if (!(*it)->getTransientWindow() && (*it)->getClientName().size()) {
				font->draw(m_title_pixmap,
									 x, (m_title_height -
											 font->getHeight((*it)->getClientName())) / 2,
									 (*it)->getClientName(), ew,
									 (TextJustify) theme->getWinFontJustify());
			}
		}

		// Restore the text color
		if (focus && (*it == m_client)) {
			font->setColor(theme->getWinFocusedText());
		}
	}

	XSetWindowBackgroundPixmap(dpy, m_title, m_title_pixmap);
	XClearWindow(dpy, m_title);
}

//! @fn    void setFocus(bool focus)
//! @brief Redraws the titlebar and borders.
void
Frame::setFocus(bool focus)
{
	repaint(focus);

	if (m_client->hasTitlebar())
		setButtonFocus(focus);
	if (m_client->hasBorder())
		setBorderFocus(focus);
}

//! @fn    void setBorderFocus(bool focus)
//! @brief Redraws the border.
//! @param focus Focus or on unfocus.
void
Frame::setBorderFocus(bool focus)
{
	Image **data = NULL;

	if (focus)
		data = theme->getWinFocusedBorder();
	else
		data = theme->getWinUnfocusedBorder();

	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		XSetWindowBackgroundPixmap(scr->getDisplay(),
															 m_border[i], data[i]->getPixmap());
		XClearWindow(scr->getDisplay(), m_border[i]);
	}
}

//! @fn    void setButtonFocus(bool f)
//! @brief Redraws the buttons in the titlebar.
//! @param f If f is true, the buttons becomes focused else they get unfocused
void
Frame::setButtonFocus(bool f)
{
	FrameButton::ButtonState state =
		f ? FrameButton::BUTTON_FOCUSED : FrameButton::BUTTON_UNFOCUSED;

	list<FrameButton*>::iterator it = m_button_list.begin();
	for (; it != m_button_list.end(); ++it) {
		(*it)->setState(state);
	}
}

//! @fn    void setBorder(void)
//! @brief Hides/Shows the border depending on m_client
void
Frame::setBorder(void)
{
	unsigned int width = m_client->getWidth();
	unsigned int height = m_client->getHeight() + m_title_height;
	if (m_client->hasBorder()) {
		width += borderLeft() + borderRight();
		height += borderTop() + borderBottom();
	}

	updateFrameSize(width, height);
	updateClientSize();

	XRaiseWindow(scr->getDisplay(), m_client->getWindow());
	if (m_client->hasBorder()) {
		XMoveWindow(scr->getDisplay(), m_client->getWindow(),
								borderLeft(), borderTop() + m_title_height);

		for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
			XRaiseWindow(scr->getDisplay(), m_border[i]);
		}
	} else {
		XMoveWindow(scr->getDisplay(), m_client->getWindow(),
								0, m_title_height);
		for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
			XLowerWindow(scr->getDisplay(), m_border[i]);
		}
	}
}

//! @fn    void setTitlebar(void)
//! @brief Hides/Shows the titlebar depending on m_client
//! @bug It changes the size, but nothing else happens
void
Frame::setTitlebar(void)
{
	updateTitleHeight();
}

//! @fn    void updateFrameSize(void)
//! @brief Resizes the frame and the frame title to the current size.
void
Frame::updateFrameSize(void)
{
	// Only update the title height if we have one, else we'll get a BadValue
	// error when trying to set the height to 0
	if (m_title_height) {
		if (!borderTop()) {
			XMoveResizeWindow(scr->getDisplay(), m_title,
												0, 0, m_width, m_title_height);
		} else {
			XMoveResizeWindow(scr->getDisplay(), m_title,
												borderLeft(), borderTop(),
											m_width - borderLeft() - borderRight(), m_title_height);
		}
	}

	if (m_client->isShaded()) {
		XResizeWindow(scr->getDisplay(), m_frame,
									m_width, m_title_height + borderTop() + borderBottom());
	} else {
		XResizeWindow(scr->getDisplay(), m_frame, m_width, m_height);
	}

	if (m_client->hasTitlebar()) {
		repaint(m_client->hasFocus());
		updateButtonPosition();
	}
	if (m_client->hasBorder())
		rearangeBorderWindows();
}

//! @fn    void updateFrameSize(unsigned int w, unsigned int h)
//! @brief Resizes the frame and the frame title to the specified size.
//! @param w Width to set the frame to
//! @param h Height to set the frame to
void
Frame::updateFrameSize(unsigned int w, unsigned int h)
{
	m_width = w;
	m_height = h;

	updateFrameSize();
}

//! @fn    void updateFramePosition(void)
//! @brief Moves the frame window to the current position.
void
Frame::updateFramePosition(void)
{
	XMoveWindow(scr->getDisplay(), m_frame, m_x, m_y);
}

//! @fn    void updateFramePosition(int x, int y)
//! @brief Moves the frame window to the specified position.
//! @param x New x position
//! @param y New y position
void
Frame::updateFramePosition(int x, int y)
{
	m_x = x;
	m_y = y;
	XMoveWindow(scr->getDisplay(), m_frame, m_x, m_y);
}

//! @fn    void updateFrameGeometry(void)
//! @brief Uppdates both the frame size and positon.
void
Frame::updateFrameGeometry(void)
{
	updateFramePosition();
	updateFrameSize();
}

//! @fn    void updateFrameGeometry(void)
//! @brief Uppdates both the frame size and positon.
//! @param x New x position
//! @param y New y position
//! @param w New width
//! @param h New height
void
Frame::updateFrameGeometry(int x, int y, unsigned int w, unsigned int h)
{
	updateFramePosition(x, y);
	updateFrameSize(w, h);
}

//! @fn    void updateClientSize(void)
//! @brief Resizes the client window to the frames size
void
Frame::updateClientSize(void)
{
	if (m_client->hasBorder()) {
		m_client->resize(m_width - borderLeft() - borderRight(),
										 m_height - m_title_height - borderTop() - borderBottom());
	} else {
		m_client->resize(m_width, m_height - m_title_height);
	}
}

//! @fn    void updateClientPosition(void)
//! @brief Updates clients position variables.
void
Frame::updateClientPosition(void)
{
	m_client->move(m_x + borderLeft(), m_y + m_title_height + borderTop());
}

//! @fn    void updateClientGeometry(void)
//! @brief Updates both client position and size.
void
Frame::updateClientGeometry(void)
{
	updateClientPosition();
	updateClientSize();
}

//! @fn    void rearangeBorderWindows(void)
//! @brief Moves and resizes the border windows to fit the frame
void
Frame::rearangeBorderWindows(void)
{
	if (borderTop() > 0) {
		XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_TOP],
											borderTopLeft(), 0,
											m_width - borderTopLeft() - borderTopRight(),
											borderTop());

		XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_TOP_LEFT],
											0, 0,
											borderTopLeft(), borderTop());
		XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_TOP_RIGHT],
											m_width - borderTopRight(), 0,
											borderTopRight(), borderTop());

		if (borderLeft()) {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_LEFT],
												0, borderTop(),
												borderLeft(),
												m_height - borderTop() - borderBottom());
		}

		if (borderRight()) {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_RIGHT],
												m_width - borderRight(), borderTop(),
												borderRight(),
												m_height - borderTop() - borderBottom());
		}
	} else {
		if (borderLeft()) {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_LEFT],
												0, m_title_height,
												borderLeft(),
												m_height - m_title_height - borderBottom());
		}

		if (borderRight()) {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_RIGHT],
												m_width - borderRight(), m_title_height,
												borderRight(),
												m_height - m_title_height - borderBottom());
		}
	}


	if (borderBottom()) {
		if (m_client->isShaded()) {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM],
												borderBottomLeft(), m_title_height + borderTop(),
												m_width - borderBottomLeft() - borderBottomRight(), borderBottom());
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM_LEFT],
												0, m_title_height + borderTop(),
												borderBottomLeft(), borderBottom());
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM_RIGHT],
												m_width - borderBottomRight(), m_title_height + borderTop(),
												borderBottomRight(), borderBottom());
		} else {
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM],
												borderBottomLeft(), m_height - borderBottom(),
												m_width - borderBottomLeft() - borderBottomRight(), borderBottom());
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM_LEFT],
												0, m_height - borderBottom(),
												borderBottomLeft(), borderBottom());
			XMoveResizeWindow(scr->getDisplay(), m_border[BORDER_BOTTOM_RIGHT],
												m_width - borderBottomRight(), m_height - borderBottom(),
												borderBottomRight(), borderBottom());
		}
	}
}

//! @fn    void loadButtons(void)
//! @brief Loads the Frame's titlebar buttons, unloads allready loaded buttons
void
Frame::loadButtons(void)
{
	if (m_button_list.size())
		unloadButtons(); // unload old buttons

	m_button_width_left = m_button_width_right = 0;

	// add buttons to the frame
	list<FrameButton::ButtonData> *b_list = theme->getButtonList();
	list<FrameButton::ButtonData>::iterator it = b_list->begin();
	for (; it != b_list->end(); ++it) {
		m_button_list.push_back(new FrameButton(wm->getScreen(), this, &*it));

		if ((*it).left)
			m_button_width_left += (*it).width;
		else
			m_button_width_right += (*it).width;
	}
}

//! @fn    void unloadButtons(void)
//! @brief Unloads Frame's titelbar buttons.
void
Frame::unloadButtons(void)
{
	if (!m_button_list.size())
		return;

	list<FrameButton*>::iterator it = m_button_list.begin();
	for (; it != m_button_list.end(); ++it) {
		delete *it;
	}
	m_button_list.clear();
}

//! @fn    void updateButtonPosition(void)
//! @brief Updates the position of buttons in the titlebar
void
Frame::updateButtonPosition(void)
{
	// well, we don't want to show our buttons on transient windows
	if (m_client && m_client->getTransientWindow())
		return;

	int left = 0;
	int right = m_width;
	if (borderTop())
		right -= borderLeft() + borderRight();

	list<FrameButton*>::iterator it = m_button_list.begin();
	for (; it != m_button_list.end(); ++it) {
		if ((*it)->isLeft()) {
			(*it)->setPosition(left, 0);
			left += (*it)->getWidth();
		} else {
			right -= (*it)->getWidth();
			(*it)->setPosition(right, 0);
		}
	}
}

//! @fn    void showAllButtons(void)
void
Frame::showAllButtons(void)
{
	if (m_button_list.size())
		for_each(m_button_list.begin(), m_button_list.end(),
						 mem_fun(&FrameButton::show));
}

//! @fn    void hideAllButtons(void)
void
Frame::hideAllButtons(void)
{
	if (m_button_list.size())
		for_each(m_button_list.begin(), m_button_list.end(),
						 mem_fun(&FrameButton::hide));
}

//! @fn    void updateTitleHeight(void)
//! @brief Updates the title windows height variable.
void
Frame::updateTitleHeight(void)
{
	unsigned int before = m_title_height;
	m_title_height = m_client->calcTitleHeight();

	if (before != m_title_height) {
		if (m_title_height) {
			XRaiseWindow(scr->getDisplay(), m_title);
		} else {
			XLowerWindow(scr->getDisplay(), m_title);
		}

		XMoveWindow(scr->getDisplay(), m_client->getWindow(),
								borderLeft(), borderTop() + m_title_height);

		updateFrameSize(m_width, m_height + m_title_height - before);
	}
}

//! @fn    void fixGeometryBasedOnStrut(Strut *strut)
//! @brief Makes sure the frame doesn't cover any struts.
//! Makes sure the frame doesn't cover any struts, repositions and resizes
//! if necessary
//!
//! @param strut Strut to take in account
//! @todo Make this nice for Xinerama users
void
Frame::fixGeometryBasedOnStrut(Strut *strut)
{
	if (! strut)
		return;

	bool size_changed = false;

	// First ensure that the window will have enough room
	if (strut->west || strut->east) {
		if (m_width > (scr->getWidth() - strut->west - strut->east)) {
			m_width = scr->getWidth() - strut->west - strut->east;
			size_changed = true;
		}

		if ((m_x + m_width) > (scr->getWidth() - strut->east))
			m_x = scr->getWidth() - m_width - strut->east;
		if (m_x < (signed) strut->west)
			m_x = strut->west;
	}

	if (strut->north || strut->south) {
		if (m_height > (scr->getHeight() - strut->north - strut->south)) {
			m_height = scr->getHeight() - strut->north - strut->south;
			size_changed = true;
		}

		if ((m_y + m_height) > (scr->getHeight() - strut->south))
			m_y = scr->getHeight() - m_height - strut->south;
		if (m_y < (signed) strut->north)
			m_y = strut->north;
	}

	if(size_changed) {
		updateFrameGeometry();
		updateClientGeometry();
	} else {
		updateFramePosition();
		updateClientPosition();
	}
}

//! @fn    Action* handleButtonEvent(XButtonEvent *e)
//! @brief Handle buttons presses.
//! Handle button presses, first searches for titlebar buttons if no were
//! found titlebar actions is performed
//!
//! @param e XButtonEvent to examine
Action*
Frame::handleButtonEvent(XButtonEvent *e)
{
#ifdef MENUS
	if (e->type == ButtonPress)
		wm->getWindowMenu()->hideAll();
#endif // MENUS

	Action *action = NULL;

	// First try to do something about frame buttons
	FrameButton *button; // used for searching titlebar button
	if (m_button) {
		if (e->type == ButtonRelease) {
			if (m_button == findButton(e->subwindow)) {
				action = m_button->getActionFromButton(e->button);
				// We don't want to execute this both on press and release!
				if (action && (action->action == RESIZE)) {
					action = NULL;
				}
			}

			// restore the pressed buttons state
			if (m_client->hasFocus()) {
				m_button->setState(FrameButton::BUTTON_FOCUSED);
			}	else {
				m_button->setState(FrameButton::BUTTON_UNFOCUSED);
			}
			m_button = NULL;
		}
	} else if (e->subwindow && (button = findButton(e->subwindow))) {
		if (e->type == ButtonPress) {
			button->setState(FrameButton::BUTTON_PRESSED);

			// If the button is used for resizing the window we want to be able
			// to resize it directly when pressing the button and not have to
			// wait until we release it, therefor this execption
			if (button->getAction(e->button) == RESIZE) {
				action = button->getActionFromButton(e->button);
				if (m_client->hasFocus()) {
					button->setState(FrameButton::BUTTON_FOCUSED);
				}	else {
					button->setState(FrameButton::BUTTON_UNFOCUSED);
				}
			} else {
				m_button = button; // set pressed button
			}
		}
	} else {

		// Also remove NumLock, ScrollLock and CapsLock
		e->state &= ~scr->getNumLock() & ~scr->getScrollLock() & ~LockMask;
		// Remove the button from the state
		e->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
			& ~Button4Mask & ~Button5Mask;

		// Used to compute the pointer position on click
		// used in the motion handler when doing a window move.
		m_old_cx = m_x;
		m_old_cy = m_y;
		m_pointer_x = e->x_root;
		m_pointer_y = e->y_root;

		// Allow us to get clicks from anywhere on the window.
		XAllowEvents(scr->getDisplay(), ReplayPointer, CurrentTime);


		// Handle clicks on the Frames title
		if (e->window == m_title) {

			if (e->type == ButtonPress) {
				action =
					findMouseButtonAction(e->button, e->state, BUTTON_SINGLE,
																wm->getConfig()->getMouseFrameList());
			} else if (e->type == ButtonRelease && (e->button < 5)) {
				// Lets see if it's a DoubleClick
				if ((e->time - m_last_button_time[e->button - 1]) <
						wm->getConfig()->getDoubleClickTime()) {

					action =
						findMouseButtonAction(e->button, e->state, BUTTON_DOUBLE,
																	wm->getConfig()->getMouseFrameList());
					m_last_button_time[e->button - 1] = 0;
				} else {
					m_last_button_time[e->button - 1] = e->time;
				}
			}

			// Clicks on the Clients window
			// NOTE: If the we're matching against the subwindow we need to make
			// sure that the state is 0, meaning we didn't have any modifier
			// because if we don't care about the modifier we'll get two actions
			// performed when using modifiers.
		}	else if ((e->window == m_client->getWindow()) ||
							 (!e->state && (e->subwindow == m_client->getWindow()))) {
			if (e->type == ButtonPress) {
				action =
					findMouseButtonAction(e->button, e->state, BUTTON_SINGLE,
																wm->getConfig()->getMouseClientList());
			}
			// Border clicks that migh initiate resizing
		} else if ((e->window == m_frame) && e->subwindow &&
							 (e->subwindow != m_title)) {
			if (e->type == ButtonPress) {
				// and do the testing by looking at the subwindow instead.
				bool x = false, y = false;
				bool left = false, top = false;
				bool resize = true;

				if (e->subwindow == m_border[BORDER_TOP_LEFT])
					x = y = left = top = true;
				else if (e->subwindow == m_border[BORDER_TOP])
					y = top = true;
				else if (e->subwindow == m_border[BORDER_TOP_RIGHT])
					x = y = top = true;
				else if (e->subwindow == m_border[BORDER_LEFT])
					x = left = true;
				else if (e->subwindow == m_border[BORDER_RIGHT])
					x = true;
				else if (e->subwindow == m_border[BORDER_BOTTOM_LEFT])
					x = y = left = true;
				else if (e->subwindow == m_border[BORDER_BOTTOM])
					y = true;
				else if (e->subwindow == m_border[BORDER_BOTTOM_RIGHT])
					x = y = true;
				else
					resize = false;

				if (resize && !m_client->isShaded()) {
					doResize(left, x, top, y);
				}
			}
		}
	}

	// If we found a action, let's do something about it.
	if (action) {
		// About the ActivateClient keygrab, it's used both for clicking in
		// the frame-title to activate a client, then it doesn't have any
		// parameter. Else it activates the nr in the frame client
		if (action->action == ACTIVATE_CLIENT) {
			activateClientFromPos(e->x);
		} else if (action->action == RESIZE) {
			doResize(false, true, false, true);
		} else {
			return action;
		}
	}

	return NULL;
}


Action*
Frame::findMouseButtonAction(unsigned int button, unsigned int mod,
														 MouseButtonType type,
														 list<MouseButtonAction> *actions)
{
	if (!actions)
		return NULL;

	list<MouseButtonAction>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->button == button) && (it->mod == mod) && (it->type == type)) {
			return &*it;
		}
	}

	return NULL;
}

//! @fn    void handleMotionNotifyEvent(XMotionEvent *ev)
//! @brief Handles Motion Events, if we have a button pressed nothing happens,
Action*
Frame::handleMotionNotifyEvent(XMotionEvent *ev)
{
	// This is true when we have a title button pressed and then we don't want
	// to be able to drag windows around, therefore we ignore the event
	if (m_button)
		return NULL;

	unsigned int button = 0;

	// Figure out wich button we have pressed
	if (ev->state&Button1Mask)
		button = Button1;
	else if (ev->state&Button2Mask)
		button = Button2;
	else if (ev->state&Button3Mask)
		button = Button3;
	else if (ev->state&Button4Mask)
		button = Button4;
	else if (ev->state&Button5Mask)
		button = Button5;

	// Remove the button from the state so that I can match it with == on
	// the mousebutton mod
	ev->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;
	// Also remove NumLock, ScrollLock and CapsLock
	ev->state &= ~scr->getNumLock() & ~scr->getScrollLock() & ~LockMask;

	Action *action = NULL;
	if (ev->window == m_title) {
		action = findMouseButtonAction(button, ev->state, BUTTON_MOTION,
																	 wm->getConfig()->getMouseFrameList());
	} else if (ev->window == m_client->getWindow()) {
		action = findMouseButtonAction(button, ev->state, BUTTON_MOTION,
																	 wm->getConfig()->getMouseClientList());
	}

	if (action) {
		if (action->action == MOVE) {
			doMove(ev);
		} else if (action->action == GROUPING_DRAG) {
			clientGroupingDrag(ev);
		} else {
			return action;
		}
	}

	return NULL;
}

//! @fn    void doMove(XMotionEvent *ev)
//! @brief Starts moving the window based on the XMotionEvent
void
Frame::doMove(XMotionEvent *ev)
{
	if (!ev)
		return;

	int status =
		XGrabPointer(scr->getDisplay(), scr->getRoot(), false,
								 ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
								 GrabModeAsync, GrabModeAsync, None,
								 wm->getCursors()[WindowManager::MOVE_CURSOR], CurrentTime);

	if (status != GrabSuccess)
		return;

	Config *cfg = wm->getConfig(); // convinience
	int resistance = signed(cfg->getWorkspaceWarp()); // convinience

	char buf[32];
	snprintf(buf, sizeof(buf), "%dx%d @ %dx%d", m_width, m_height, m_x, m_y);
	wm->drawInStatusWindow(buf); // to make things less flickery
	wm->showStatusWindow();

	if (!cfg->getOpaqueMove()) {
		XGrabServer(scr->getDisplay());
		drawWireframe(); // so that first clear will work
	}

	XEvent e;
	while (true) { // breaks when we get an ButtonRelease event
		XMaskEvent(scr->getDisplay(), ButtonReleaseMask|PointerMotionMask, &e);

		switch (e.type) {
		case MotionNotify:
			if (!cfg->getOpaqueMove())
				drawWireframe(); // clear

			m_x = m_old_cx + (e.xmotion.x_root - m_pointer_x);
			m_y = m_old_cy + (e.xmotion.y_root - m_pointer_y);

			// I prefer having the EdgeSnap after the FrameSnap, feels better IMHO
			if (wm->getConfig()->getFrameSnap())
				wm->getWorkspaces()->checkFrameSnap(m_x, m_y,
																						this, wm->getActiveWorkspace());
			if(wm->getConfig()->getEdgeSnap())
				checkEdgeSnap();

			snprintf(buf, sizeof(buf), "%dx%d @ %dx%d", m_width, m_height, m_x, m_y);
			wm->drawInStatusWindow(buf);

			if (cfg->getOpaqueMove()) {
				updateFramePosition();
				updateClientPosition();
			} else {
				drawWireframe();
			}

			if (resistance) {
				bool left = false;
				bool right = false;
				unsigned int ws = wm->getActiveWorkspace(); // convinience

				int num_ws = wm->getWorkspaces()->getNumWorkspaces() - 1;
				int new_ws = ws;

				left = (e.xmotion.x_root <= resistance);
				if (!left)
					right = (e.xmotion.x_root >= signed(scr->getWidth() - resistance));

				new_ws = (left ? (ws - 1) : (ws + 1));
				if (cfg->getWrapWorkspaceWarp()) {
					if (new_ws < 0)
						new_ws = num_ws;
					else if (new_ws > num_ws)
						new_ws = 0;
				} else if ((new_ws < 0) || (new_ws > num_ws)) {
					break;
				}

				// found a workspace to warp to
				if (left || right) {
					// If we draw wireframe, we need to ungrab when changing workspace
					if (!cfg->getOpaqueMove()) {
						drawWireframe(); // clear
					}

					unsigned int warp = resistance * 2;

					// warp the pointer
					XWarpPointer(scr->getDisplay(), None, scr->getRoot(),
											 0, 0, 0, 0,
											 left ? scr->getWidth() - warp : warp,
											 e.xmotion.y_root);

					// warp workspace
					wm->setWorkspace(new_ws, false); // do not focus

					// set new position of the frame
					m_x = left
						? (m_old_cx + scr->getWidth() - warp - m_pointer_x)
						: (m_old_cx + warp - m_pointer_x);
					m_y = m_old_cy + e.xmotion.y_root - m_pointer_y;

					updateFramePosition();
					updateClientPosition();

					// warp client to the new workspace
					setWorkspace(wm->getActiveWorkspace());

					// make sure the client has focus
					m_client->getFrame()->setFocus(true);
					m_client->giveInputFocus();

					// make sure the status window is ontop of the frame
					wm->showStatusWindow();

					if (!cfg->getOpaqueMove()) {
						drawWireframe();
					}
				}
			}

			break;

		case ButtonRelease:
			if (!cfg->getOpaqueMove()) {
				drawWireframe(); // clear

				XSync(scr->getDisplay(), false);
				XUngrabServer(scr->getDisplay());

				updateFramePosition();
				updateClientPosition();
			}

			wm->hideStatusWindow();

			XUngrabPointer(scr->getDisplay(), CurrentTime);
			return;

			break;
		}
	}
}

//! @fn    void clientGroupingDrag(XMotionEvent *ev)
void
Frame::clientGroupingDrag(XMotionEvent *ev)
{
	int o_x, o_y;
	o_x = ev ? ev->x_root : 0;
	o_y = ev ? ev->y_root : 0;

	string name("Grouping ");
	if (m_client->getClientName().size()) {
		name += m_client->getClientName();
	} else {
		name += "No Name";
	}

	int status =
		XGrabPointer(scr->getDisplay(), scr->getRoot(), false,
								 ButtonReleaseMask|PointerMotionMask,
								 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
	if (status != GrabSuccess)
		return;

	wm->drawInStatusWindow(name, o_x, o_y); // to make things less flickery
	wm->showStatusWindow();

	XEvent e;
	while (true) { // this breaks when we get an button release
		XMaskEvent(scr->getDisplay(), PointerMotionMask|ButtonReleaseMask, &e);

		switch (e.type)  {
		case MotionNotify:
			// update the position
			o_x = e.xmotion.x_root;
			o_y = e.xmotion.y_root;

			wm->drawInStatusWindow(name, o_x, o_y);
			break;

		case ButtonRelease:
			wm->hideStatusWindow();
			XUngrabPointer(scr->getDisplay(), CurrentTime);

			Window win;
			int x, y;
			// find the frame we dropped the client on
			XTranslateCoordinates(scr->getDisplay(), scr->getRoot(), scr->getRoot(),
														e.xmotion.x_root, e.xmotion.y_root,
														&x, &y, &win);

			Client *client = NULL;
			if (m_client_list.size() > 1) {
				client = m_client;
				removeClient(m_client);

				client->move(e.xmotion.x_root, e.xmotion.y_root);

				Frame *frame = new Frame(wm, client);
				client->setFrame(frame);

				// make sure the client ends up on the current workspace
				client->setWorkspace(wm->getActiveWorkspace());

				// make sure it get's focus
				client->giveInputFocus();

			} else if ((client = wm->findClient(win)) && !clientIsInFrame(client) &&
								 (client->getLayer() <= WIN_LAYER_ONTOP) &&
								 (client->getLayer() >= WIN_LAYER_BELOW) &&
								 (client->getFrame())) {
				Client *ins_client = m_client;
				removeClient(m_client);
				client->getFrame()->insertClient(ins_client);

				// make sure it get's repainted
				ins_client->getFrame()->repaint(true);
			}

			return;
		}
	}
}

//! @fn    void drawWireframe(void)
//! @brief Draws the outline of the frame
void
Frame::drawWireframe(void)
{
	Display *dpy = scr->getDisplay();

	if (m_client->isShaded()) {
		XDrawRectangle(dpy, scr->getRoot(), theme->getInvertGC(),
									 m_x, m_y,
									 m_width, m_title_height + borderTop() + borderBottom());
	} else { // not shaded
		XDrawRectangle(dpy, scr->getRoot(), theme->getInvertGC(),
									 m_x, m_y,
									 m_width, m_height);
	}
}

//! @fn    void doResize(bool left, bool x, bool top, bool y)
//! @brief Resizes the frame by handling MotionNotify events.
void
Frame::doResize(bool left, bool x, bool top, bool y)
{
	int status =
		XGrabPointer(scr->getDisplay(), scr->getRoot(),
								 False, ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
								 GrabModeAsync, GrabModeAsync, None,
								 wm->getCursors()[WindowManager::RESIZE_CURSOR], CurrentTime);

	if (status != GrabSuccess)
		return;

	Config *cfg = wm->getConfig(); // convinience

	// Only grab if we want to
	if (wm->getConfig()->getGrabWhenResize())
		XGrabServer(scr->getDisplay());

	if (m_client->isShaded())
		shade();

	// we use these if we aren't aloved to move in either x or y direction
	int start_x = left ? m_x : (m_x + m_width);
	int start_y = top ? m_y : (m_y + m_height);

	// the basepoint of our window
	m_old_cx = left ? (m_x + m_width) : m_x;
	m_old_cy = top ? (m_y + m_height) : m_y;

	int pointer_x = m_x, pointer_y = m_y;
	wm->getMousePosition(&pointer_x, &pointer_y);

	int warp_x, warp_y;
	if (x)
	  warp_x = left ? 0 : m_width;
	else
	  warp_x = pointer_x - m_x;
	if (y)
	  warp_y = top ? 0 : m_height;
	else
	  warp_y = pointer_y - m_y;

	XWarpPointer(scr->getDisplay(), None, m_frame, 0, 0, 0, 0, warp_x, warp_y);

	int new_x, new_y;

	char buf[32];
	snprintf(buf, sizeof(buf), "%dx%d @ %dx%d", m_width, m_height, m_x, m_y);
	wm->drawInStatusWindow(buf); // makes the window resize before we map
	wm->showStatusWindow();

	// draw the wire frame of the window, so that the first clear works.
	if (!cfg->getOpaqueResize())
		drawWireframe();

	unsigned int old_width = m_width;
	unsigned int old_height = m_height;

	XEvent ev;
	while (true) { // breaks when we get an ButtonRelease event
		XMaskEvent(scr->getDisplay(),
							 ButtonPressMask|ButtonReleaseMask|PointerMotionMask, &ev);

		switch (ev.type) {
		case MotionNotify:
			if (!cfg->getOpaqueResize())
				drawWireframe(); // clear

			new_x = x ? ev.xmotion.x : start_x;
			new_y = y ? ev.xmotion.y : start_y;

			recalcResizeDrag(new_x, new_y, left, top);

			snprintf(buf, sizeof(buf), "%dx%d @ %dx%d", m_width, m_height, m_x, m_y);
			wm->drawInStatusWindow(buf);

			// we need to do this everytime, not as in opaque when we update
			// when something has changed
			if (!cfg->getOpaqueResize())
				drawWireframe();

			// Here's the deal, we try to be a bit conservative with redraws
			// when resizing, especially as drawing scaled pixmaps is _slow_
			if ((old_width != m_width) || (old_height != m_height)) {
				if (cfg->getOpaqueResize()) {
					updateFrameGeometry();
					updateClientGeometry();
				}
				old_width = m_width;
				old_height = m_height;
			}
		break;

		case ButtonRelease:
			if (!cfg->getOpaqueResize())
				drawWireframe(); // clear

			wm->hideStatusWindow();

			XUngrabPointer(scr->getDisplay(), CurrentTime);
			XSync(scr->getDisplay(), false);

			updateFrameGeometry();
			updateClientGeometry();

			if (wm->getConfig()->getGrabWhenResize()) {
				XSync(scr->getDisplay(), false);
				XUngrabServer(scr->getDisplay());
			}
			return;
		}
	}
}

//! @fn    void recalcResizeDrag(int nx, int ny, bool left, bool top)
//! @brief Updates the width, height of the frame when resizing it.
void
Frame::recalcResizeDrag(int nx, int ny, bool left, bool top)
{
	unsigned int brdr_lr = borderLeft() + borderRight();
	unsigned int brdr_tb = borderTop() + borderBottom();

	if (left) {
		if (nx >= (signed) (m_old_cx - brdr_lr))
			nx = m_old_cx - brdr_lr - 1;
	} else {
		if (nx <= (signed) (m_old_cx + brdr_lr))
			nx = m_old_cx + brdr_lr + 1;
	}

	if (top) {
		if (ny >= (signed) (m_old_cy - m_title_height - brdr_tb))
			ny = m_old_cy - m_title_height - brdr_tb - 1;
	} else {
		if (ny <= (signed) (m_old_cy + m_title_height + brdr_tb))
			ny = m_old_cy + m_title_height + brdr_tb + 1;
	}

	unsigned int width = left ? (m_old_cx - nx) : (nx - m_old_cx);
	unsigned int height = top ? (m_old_cy - ny) : (ny - m_old_cy);

	if(width > scr->getWidth())
		width = scr->getWidth();
	if(height > scr->getHeight())
		height = scr->getHeight();

	width -= brdr_lr;
	height -= brdr_tb;
	height -= m_title_height;
	m_client->getIncSize(&width, &height, width, height);

	const XSizeHints *hints = m_client->getXSizeHints();
	// check so we aren't overriding min or max size
	if (hints->flags & PMinSize) {
		if ((signed) width < hints->min_width)
			width = hints->min_width;
		if ((signed) height < hints->min_height)
			height = hints->min_height;
	}

	if (hints->flags & PMaxSize) {
		if ((signed) width > hints->max_width)
			width = hints->max_width;
		if ((signed) height > hints->max_height)
			height = hints->max_height;
	}

	m_width = width + brdr_lr;
	m_height = height + m_title_height + brdr_tb;

	m_x = left ? (m_old_cx - m_width) : m_old_cx;
	m_y = top ? (m_old_cy - m_height) : m_old_cy;
}

//! @fn    void hide(void)
//! @brief Hides the frame, title _but_ not the client's
void
Frame::hide(void)
{
	if (m_is_hidden)
		return;
	m_is_hidden = true;

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		if (!(*it)->isHidden())
			(*it)->hide();
	}

	XUnmapWindow(scr->getDisplay(), m_frame);
}

//! @fn    void hideClient(Client *client)
//! @brief Hides the client client
//! Hides the Client client, if all clients are hidden we also
//! hide the frame else we activate a non hidden client in the frame.
//! @param client Client in the frame to hide.
void
Frame::hideClient(Client *client)
{
	if (!client || !m_client_list.size())
		return;

	if (!client->isHidden())
		client->hide();

	// if we have more than one clients in the frame, we'll try to find
	// another client to activate if this was the active one
	if (m_client_list.size() > 1) {
		if (client == m_client) {
			Client *new_client = NULL;

			vector<Client*>::iterator it = m_client_list.begin();
			for (it = m_client_list.begin(); it != m_client_list.end(); ++it) {
				if ((*it != m_client) && !(*it)->isHidden()) {
					new_client = *it;
					break;
				}
			}

			if (new_client) {
				activateClient(new_client);
			} else {
				hide(); // all the clients are iconified, let's hide the window
			}
		}
	} else {
		hide(); // we only have 1 client
	}
}

//! @fn    void unhide(void)
//! @brief Unhides the Frame.
//! @param restack Defaults to true
void
Frame::unhide(bool restack)
{
	if (!m_is_hidden)
		return;

	m_is_hidden = false;
	XMapWindow(scr->getDisplay(), m_frame);

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		if ((*it)->isHidden() && !(*it)->isIconified())
			(*it)->unhide();
	}

	if (restack)
		raise(); // to handle stacking
}

//! @fn    void unhideClient(Client *client)
//! @brief Unhides the Client client, if the frame is hidden it gets unhidden too.
//! @param client Client in the frame to unhide.
void
Frame::unhideClient(Client *client)
{
	if (!client || !m_client_list.size())
		return;

	if (m_is_hidden)
		unhide();

	client->unhide();
	if (client != m_client)
		activateClient(client);
}

void Frame::iconifyAll(void)
{
	hide();

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		(*it)->iconify(); // do not send hideClient to this frame
	}
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Moves the frame and it's clients to the workspace workspace
void
Frame::setWorkspace(unsigned int workspace)
{
	if (workspace >= wm->getWorkspaces()->getNumWorkspaces())
		return;

	Client *focus_client = m_client;

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		(*it)->setWorkspace(workspace);
	}

	activateClient(focus_client, false); // do not focus
}

//! @fn    void move(int x, int y)
//! @brief Moves the frame to the position x,y.
void
Frame::move(int x, int y)
{
	if (x > (signed) scr->getWidth())
		x = scr->getWidth() - 10;
	else if ((x + (signed) m_width) < 0)
		x = 10 - m_width;
	if (y > (signed) scr->getHeight())
		y = scr->getHeight() - 10;
	else if ((y + (signed) m_height) < 0)
		y = 10 - m_height;

	m_x = x;
	m_y = y;

	updateFramePosition();
	updateClientPosition();
}

//! @fn    moveToCorner(Corner corner)
//! @brief Moves the frame to one of the corners of the screen.
void
Frame::moveToCorner(Corner corner)
{
#ifdef XINERAMA
		ScreenInfo::HeadInfo head;
		unsigned int head_nr = 0;

		if (scr->hasXinerama()) {
			head_nr = scr->getHead(m_x + (m_width / 2), m_y + (m_height / 2));
		}
		scr->getHeadInfo(head_nr, head);

	switch (corner) {
	case TOP_LEFT:
		m_x = head.x;
		m_y = head.y;
		break;
	case TOP_RIGHT:
		m_x = head.x + head.width - m_width;
		m_y = head.y;
		break;
	case BOTTOM_LEFT:
		m_x = head.x;
		m_y = head.y + head.height - m_height;
		break;
	case BOTTOM_RIGHT:
		m_x = head.x + head.width - m_width;
		m_y = head.y + head.height - m_height;
		break;
	default:
		// DO NOTHING
		break;
	}
#else // !XINERAMA
	switch (corner) {
	case TOP_LEFT:
		m_x = 0;
		m_y = 0;
		break;
	case TOP_RIGHT:
		m_x = scr->getWidth() - m_width;
		m_y = 0;
		break;
	case BOTTOM_LEFT:
		m_x = 0;
		m_y = scr->getHeight() - m_height;
		break;
	case BOTTOM_RIGHT:
		m_x = scr->getWidth() - m_width;
		m_y = scr->getHeight() - m_height;
		break;
	default:
		// DO NOTHING
		break;
	}
#endif // XINERAMA

	updateFramePosition();
	updateClientPosition();
}

//! @fn    void resizeHorizontal(int size_diff)
//! @brief In/Decreases the size of the window horizontally
//! @param size_diff How much to resize
void
Frame::resizeHorizontal(int size_diff)
{
	if (!size_diff)
		return;

	if (m_client->isShaded())
		shade();

	unsigned int width = m_width;
	unsigned int client_width = m_client->getWidth();

	XSizeHints *size_hints = m_client->getXSizeHints();

	// If we have a a ResizeInc hint set, let's use it instead of the param
	if (size_hints->flags&PResizeInc) {
		if (size_diff > 0) {	// increase the size
			m_width += size_hints->width_inc;
			// only decrease if we are over 0 pixels in the end
		} else if (m_client->getWidth() > (unsigned) size_hints->width_inc) {
			m_width -= size_hints->width_inc;
		}
	} else if (((signed) m_client->getWidth() + size_diff) > 0) {
		m_width += size_diff;
	}

	if (width > m_width)
		client_width -= width - m_width;
	else if (width < m_width)
		client_width += m_width - width;

	// check if we overide min/max size hints
	if (size_diff > 0) {
		if ((size_hints->flags&PMaxSize) &&
				(client_width > (unsigned) size_hints->max_width)) {
			m_width -= client_width - size_hints->max_width;
		}
	} else {
		if ((size_hints->flags&PMinSize) &&
				(client_width < (unsigned) size_hints->min_width)) {
			m_width += size_hints->min_width - client_width;
		}
	}

	updateFrameGeometry();
	updateClientGeometry();
}

//! @fn    void resizeVertical(int size_diff)
//! @brief In/Decreases the size of the window vertically
//! @param size_diff How much to resize
void
Frame::resizeVertical(int size_diff)
{
	if (!size_diff)
		return;

	if (m_client->isShaded())
		shade();

	unsigned int height = m_height;
	unsigned int client_height = m_client->getHeight();

	XSizeHints *size_hints = m_client->getXSizeHints();

	// If we have a a ResizeInc hint set, let's use it instead of the param
	if (size_hints->flags&PResizeInc) {
		if (size_diff > 0) {	// increase the size
			m_height += size_hints->height_inc;
			// only decrease if we are over 0 pixels in the end
		} else if (m_client->getHeight() > (unsigned) size_hints->height_inc) {
			m_height -= size_hints->height_inc;
		}
	} else if (((signed) m_client->getHeight() + size_diff) > 0) {
		m_height += size_diff;
	}

	if (height > m_height)
		client_height -= height - m_height;
	else if (height < m_height)
		client_height += m_height - height;

	// check if we overide min/max size hints
	if (size_diff > 0) {
		if ((size_hints->flags&PMaxSize) &&
				(client_height > (unsigned) size_hints->max_height)) {
			m_height -= client_height - size_hints->max_height;
		}
	} else {
		if ((size_hints->flags&PMinSize) &&
				(client_height < (unsigned) size_hints->min_height)) {
			m_height += size_hints->min_height - client_height;
		}
	}

	updateFrameSize();
	updateClientSize();
}

//! @fn    void raise(bool restack)
//! @brief Moves the frame ontop of all windows
//! @param restack Defaults to true
void
Frame::raise(bool restack)
{
	// we shouldn't raise these windows
	if (m_client->getLayer() <= WIN_LAYER_BELOW)
		return;

	wm->getWorkspaces()->raiseClient(m_client, m_client->onWorkspace());
}

//! @fn    void lower(bool restack)
//! @brief Moves the frame beneath all windows
//! @param restack Defaults to true
void
Frame::lower(bool restack)
{
	wm->getWorkspaces()->lowerClient(m_client, m_client->onWorkspace());
}

//! @fn    void shade(void)
//! @brief Toggles the frames shade state
void
Frame::shade(void)
{
	// we won't shade windows like gkrellm and xmms
	if (!m_client->hasTitlebar())
		return;

	if (m_client->isShaded()) {
		m_client->setShade(false);
		updateFrameSize();
		rearangeBorderWindows();
	} else {
		m_client->setShade(true);
		updateFrameSize();
		rearangeBorderWindows();
	}

	m_client->updateWmStates();
}

//! @fn    void maximize(void)
//! @brief Toggles current clients max size
void
Frame::maximize(void)
{
	// We don't maximize transients
	if(m_client->getTransientWindow())
		return;

	if(m_client->isShaded())
		shade();

	if(!m_client->isMaximized()) {
#ifdef XINERAMA
		ScreenInfo::HeadInfo head;
		unsigned int head_nr = 0;

		if (scr->hasXinerama()) {
			head_nr = scr->getHead(m_x + (m_width / 2), m_y + (m_height / 2));
		}
		scr->getHeadInfo(head_nr, head);
#endif // XINERAMA
		m_old_x = m_x;
		m_old_y = m_y;
		m_old_width = m_width;
		m_old_height = m_height;

		// Check to see if this client sets its max size property.
		// If so don't maximize it past that size.
		if (m_client->getXSizeHints()->flags&PMaxSize) {
			m_width = m_client->getXSizeHints()->max_width;
			m_height = m_client->getXSizeHints()->max_height + m_title_height;
#ifdef XINERAMA
			m_x = head.x;
			m_y = head.y;
			if (m_width > head.width)
				m_width = head.width;
			if (m_height > head.height)
				m_height = head.height;
#else // !XINERAMA
			m_x = 0;
			m_y = 0;
			if (m_width > scr->getWidth())
				m_width = scr->getWidth();
			if (m_height > scr->getHeight())
				m_height = scr->getHeight();
#endif // XINEAMA
		} else {
#ifdef XINERAMA
			m_x = head.x;
			m_y = head.y;
			m_width = head.width;
			m_height = head.height;
#else // !XINERAMA
			m_x = 0;
			m_y = 0;
			m_width = scr->getWidth();
			m_height = scr->getHeight();
#endif // XINERAMA
		}

		m_client->setMaximized(true);
		m_client->setVertMaximized(true);
		m_client->setHorizMaximized(true);

	} else { // unmaximize
		m_x = m_old_x;
		m_y = m_old_y;
		m_width = m_old_width;
		m_height = m_old_height;

		m_client->setMaximized(false);
		m_client->setVertMaximized(false);
		m_client->setHorizMaximized(false);

		if(m_client->isShaded()) {
			m_client->setShade(false);
		}
	}

	updateFrameGeometry();
	updateClientGeometry();

	fixGeometryBasedOnStrut(wm->getMasterStrut());

	m_client->updateWmStates();
}

//! @fn    void maximizeHorizontal(void)
//! @brief Toggles current clients horizontal max size
void
Frame::maximizeHorizontal(void)
{
	if(m_client->getTransientWindow())
		return;

	if(m_client->isShaded())
		shade();

	if(! m_client->isMaximizedHorizontal()) {
		m_old_x = m_x;
		m_old_width = m_width;

		// Check to see if this client sets its max size property.
		// If so don't maximize it past that size.
		if (m_client->getXSizeHints()->flags & PMaxSize) {
			m_width = m_client->getXSizeHints()->max_width;

			updateFrameSize();
		} else {
#ifdef XINERAMA
			ScreenInfo::HeadInfo head;
			unsigned int head_nr = 0;

			// We take the middle of the window, makes it feel more
			// natural, atleast according to me! :)
			if (scr->hasXinerama()) {
				head_nr =
					scr->getHead(m_x + (m_width / 2), m_y + (m_height / 2));
			}
			scr->getHeadInfo(head_nr, head);

			m_x = head.x;
			m_width = head.width;
#else // !XINERAMA
			m_x = 0;
			m_width = scr->getWidth();
#endif // XINERAMA
		}

		m_client->setHorizMaximized(true);
		if (m_client->isMaximizedVertical())
			m_client->setMaximized(true);

	} else { // unmaximize
		m_x = m_old_x;
		m_width = m_old_width;

		m_client->setHorizMaximized(false);
		m_client->setMaximized(false);

		if(m_client->isShaded()) {
			m_client->setShade(false);
		}
	}

	updateFrameGeometry();
	updateClientGeometry();

	fixGeometryBasedOnStrut(wm->getMasterStrut());

	m_client->updateWmStates();
}

//! @fn    void maximizeVertical(void)
//! @brief Toggles current clients vertical max size
void
Frame::maximizeVertical(void)
{
	if(m_client->getTransientWindow())
		return;

	if(m_client->isShaded())
		shade();

	if(!m_client->isMaximizedVertical()) {
		m_old_y = m_y;
		m_old_height = m_height;

		// Check to see if this client sets its max size property.
		// If so don't maximize it past that size.
		if (m_client->getXSizeHints()->flags & PMaxSize) {
			m_height = m_client->getXSizeHints()->max_height + m_title_height;

			updateFrameSize();
		} else {
#ifdef XINERAMA
			ScreenInfo::HeadInfo head;
			unsigned int head_nr = 0;

			// We take the middle of the window, makes it feel more
			// natural, atleast according to me! :)
			if (scr->hasXinerama()) {
				head_nr =
					scr->getHead(m_x + (m_width / 2), m_y + (m_height / 2));
			}
			scr->getHeadInfo(head_nr, head);

			m_y = head.y;
			m_height = head.height;
#else // !XINERAMA
			m_y = 0;
			m_height = scr->getHeight();
#endif // XINERAMA
		}

		m_client->setVertMaximized(true);
		if (m_client->isMaximizedHorizontal())
			m_client->setMaximized(true);

	} else { // unmaximize
		m_y = m_old_y;
		m_height = m_old_height;

		m_client->setVertMaximized(false);
		m_client->setMaximized(false);

		if(m_client->isShaded()) {
			m_client->setShade(false);
		}
	}

	updateFrameGeometry();
	updateClientGeometry();

	fixGeometryBasedOnStrut(wm->getMasterStrut());

	m_client->updateWmStates();
}

#ifdef MENUS
void
Frame::showWindowMenu(void)
{
	if (wm->getWindowMenu()->isVisible()) {
		wm->getWindowMenu()->hideAll();
	} else {
		wm->getWindowMenu()->setThisClient(m_client);
		wm->getWindowMenu()->showUnderMouse();
	}
}
#endif // MENUS

void
Frame::checkEdgeSnap(void)
{
	int snap = wm->getConfig()->getEdgeSnap(); // convinience
	unsigned int height = m_client->isShaded()
		? (m_title_height + borderTop() + borderBottom())
		: m_height;

#ifdef XINERAMA
	ScreenInfo::HeadInfo head;
	scr->getHeadInfo(scr->getHead(m_x, m_y), head);

	// Move beyond edges of screen
	if (m_x == (signed) (head.x + head.width - m_width)) {
		m_x = head.x + head.width - m_width + 1;
	} else if (m_x == head.x) {
		m_x = head.x - 1;
	}

	if (m_y == (signed) (head.y + head.height - snap)) {
		m_y = head.y + head.height - snap - 1;
	} else if (m_y == head.y) {
		m_y = head.y - 1;
	}

	// Snap to edges of screen
	if ((m_x >= (head.x - snap)) && (m_x <= (head.x + snap))) {
		m_x = head.x;
	} else if ((m_x + m_width) >= (head.x + head.width - snap) &&
						 ((m_x + m_width) <= (head.x + head.width + snap))) {
		m_x = head.x + head.width - m_width;
	}

	if ((m_y >= (head.y - snap)) && (m_y <= (head.y + snap))) {
		m_y = head.y;
	} else if (((m_y + height) >= (head.y + head.height - snap)) &&
						 ((m_y + height) <= (head.y + head.height + snap))) {
		m_y = head.y + head.height - height;
	}
#else //!XINERAMA

	int west = wm->getMasterStrut()->west;
	int east = wm->getMasterStrut()->east;
	int north = wm->getMasterStrut()->north;
	int south = wm->getMasterStrut()->south;

	int xres = scr->getWidth() - east;
	int yres = scr->getHeight() - south;

	// Snap to edges of screen
	if((m_x >= (west - snap)) && (m_x <= (west + snap))) {
		m_x = west;
	} else if(((signed) (m_x + m_width) >= (xres - snap)) &&
						((signed) (m_x + m_width) <= (xres + snap))) {
		m_x = xres - m_width;
	}

	if((m_y >= (north - snap)) && (m_y <= (north + snap))) {
		m_y = north;
	} else if(((signed) (m_y + height) >= (yres - snap)) &&
						((signed) (m_y + height) <= (yres + snap))) {
		m_y = yres - height;
	}
#endif // XINERAMA
}
