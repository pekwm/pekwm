//
// SystrayWidget.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <limits>

extern "C" {
#include <assert.h>
}

#include "Compat.hh"
#include "Debug.hh"
#include "SystrayWidget.hh"

enum XEmbedOpcode {
	XEMBED_EMBEDDED_NOTIFY = 0
};

enum SystrayOpcode {
	SYSTEM_TRAY_REQUEST_DOCK = 0,
	SYSTEM_TRAY_BEGIN_MESSAGE = 1,
	SYSTEM_TRAY_CANCEL_MESSAGE = 2
};

// SystrayWidget::Client

SystrayWidget::Client::Client(Window win, uint side)
	: PWinObj(false),
	  _xembed_version(0),
	  _xembed_flags(0)
{
	setWindow(win);
	resize(side, side);
	X11::selectInput(win, PropertyChangeMask);
}

SystrayWidget::Client::~Client()
{
}

/**
 * Return true if the client is set to be mapped/visible.
 */
bool
SystrayWidget::Client::isMapped() const
{
	return (_xembed_flags & XEMBED_FLAG_MAPPED) == XEMBED_FLAG_MAPPED;
}

void
SystrayWidget::Client::sendConfigureNotify()
{
	XConfigureEvent ev = {};

	ev.type = ConfigureNotify;
	ev.event = _window;
	ev.window = _window;
	ev.x = _gm.x;
	ev.y = _gm.y;
	ev.width = _gm.width;
	ev.height = _gm.height;

	X11::sendEvent(_window, False, StructureNotifyMask,
		       reinterpret_cast<XEvent*>(&ev));
}

// SystrayWidget

SystrayWidget::SystrayWidget(const PanelWidgetData &data,
			     const PWinObj* parent, const WidgetConfig& cfg)
	: PanelWidget(data, parent, cfg.getSizeReq(), cfg.getIf()),
	  _owner(None),
	  _owner_atom(0)
{
	// avoid duplicate mappings, only add if base PanelWidget has not
	if (_if_tfo.isFixed()) {
		pekwm::observerMapping()->addObserver(this, _observer, 100);
	}

	// owner window
	XSetWindowAttributes attrs = {0};
	attrs.event_mask = SubstructureRedirectMask;
	_owner =
		X11::createWindow(X11::getRoot(), -1, -1, 1, 1, 0,
				  CopyFromParent, InputOutput,
				  X11::getVisual(), CWEventMask, &attrs);
	if (_owner == None) {
		P_ERR("systray: failed to create owner window");
	} else {
		P_TRACE("systray: owner window " << _owner);
	}

	if (claimNetSystray()) {
		notifyClaimedSystray();
	}
}

SystrayWidget::~SystrayWidget()
{
	while (! _clients.empty()) {
		removeTrayIcon(*_clients.begin(), true);
	}
	X11::destroyWindow(_owner);

	if (_if_tfo.isFixed()) {
		pekwm::observerMapping()->removeObserver(this, _observer);
	}
}

uint
SystrayWidget::getRequiredSize(void) const
{
	uint required_size = _theme.getHeight() * numClientsMapped();
	P_TRACE("systray: required size " << required_size);
	return required_size;
}

void
SystrayWidget::move(int x)
{
	P_TRACE("systray: x: " << x << " width: " << getWidth());
	PanelWidget::move(x);

	client_it it = _clients.begin();
	for (; it != _clients.end(); ++it) {
		if ((*it)->isMapped()) {
			(*it)->move(x, 0);
			x += _theme.getHeight();
		} else {
			P_TRACE("systray: skip unmapped client "
				<< (*it)->getWindow());
		}
	}
}

/**
 * Custom X event handling for systray to support interaction with systray
 * clients.
 */
bool
SystrayWidget::handleXEvent(XEvent* ev)
{
	P_TRACE("systray: handle event " << ev->type);
	switch (ev->type) {
	case DestroyNotify:
		return handleDestroyNotify(&ev->xdestroywindow);
	case ClientMessage:
		return handleClientMessage(&ev->xclient);
	case PropertyNotify:
		return handlePropertyNotify(&ev->xproperty);
	default:
		return false;
	}
}

SystrayWidget::Client*
SystrayWidget::addTrayIcon(Window win)
{
	P_TRACE("systray: add tray icon " << win);

	int x = getX() + (numClientsMapped() * _theme.getHeight());
	int y = 0;
	SystrayWidget::Client* client =
		new SystrayWidget::Client(win, _theme.getHeight());
	readXEmbedInfo(client);

	X11::reparentWindow(client->getWindow(), _parent->getWindow(), x, y);
	client->sendConfigureNotify();
	if (client->isMapped()) {
		P_TRACE("systray: mapping client " << client->getWindow());
		X11::mapRaised(client->getWindow());
	}

	_clients.push_back(client);
	sendRequiredSizeChanged();

	return client;
}

/**
 * Send message to client notifying it about a successful dock request.
 */
void
SystrayWidget::notifyTrayIcon(SystrayWidget::Client* client)
{
	XClientMessageEvent ev = {0};
	ev.type = ClientMessage;
	ev.window = client->getWindow();
	ev.message_type = X11::getAtom(XEMBED);
	ev.format = 32;
	ev.data.l[0] = CurrentTime;
	ev.data.l[1] = XEMBED_EMBEDDED_NOTIFY;
	ev.data.l[2] = std::min(XEMBED_VERSION, client->getXEmbedVersion());
	ev.data.l[3] = _parent->getWindow();
	ev.data.l[4] = 0;
	X11::sendEvent(X11::getRoot(), False, NoEventMask,
		       reinterpret_cast<XEvent*>(&ev));
}

void
SystrayWidget::removeTrayIcon(SystrayWidget::Client* client, bool reparent)
{
	P_TRACE("systray: remove tray icon " << client->getWindow());

	client_it it = std::find(_clients.begin(), _clients.end(), client);
	assert(it != _clients.end());

	if (reparent) {
		X11::reparentWindow((*it)->getWindow(), X11::getRoot(),
				    0 - (*it)->getWidth(),
				    0 - (*it)->getHeight());
	}

	delete *it;
	_clients.erase(it);
}

SystrayWidget::Client*
SystrayWidget::findClient(Window win) const
{
	client_cit it = _clients.begin();
	for (; it != _clients.end(); ++it) {
		if ((*it)->getWindow() == win) {
			return *it;
		}
	}
	return nullptr;
}

int
SystrayWidget::numClientsMapped() const
{
	int num_mapped = 0;
	client_cit it = _clients.begin();
	for (; it != _clients.end(); ++it) {
		if ((*it)->isMapped()) {
			num_mapped++;
		}
	}
	return num_mapped;
}

/**
 * Claim ownership of _NET_SYSTEM_TRAY_S0, required step to make systray
 * clients find the current active tray.
 */
bool
SystrayWidget::claimNetSystray()
{
	std::string default_str = std::to_string(X11::getScreenNum());
	std::string session_name("_NET_SYSTEM_TRAY_S" + default_str);
	_owner_atom = X11::getAtomId(session_name);
	Window owner = X11::getSelectionOwner(_owner_atom);
	if (owner && owner != _owner) {
		P_DBG("systray: " << session_name << " already claimed by "
		      << owner << ", not active");
		return false;
	}

	X11::setSelectionOwner(_owner_atom, _owner);
	owner = X11::getSelectionOwner(_owner_atom);
	if (owner == _owner) {
		P_TRACE("systray: " << _owner << " claimed ownership of "
			<< session_name);
		return true;
	} else {
		P_DBG("systray: failed to claim ownership ownership of "
		      << session_name << ", owned by " << owner);
		return false;
	}
}

/**
 * Send event to the root window to notify systray clients that a new
 * systray is available.
 */
void
SystrayWidget::notifyClaimedSystray()
{
	XClientMessageEvent ev = {0};
	ev.type = ClientMessage;
	ev.window = X11::getRoot();
	ev.message_type = X11::getAtom(MANAGER);
	ev.format = 32;
	ev.data.l[0] = X11::getLastEventTime();
	ev.data.l[1] = _owner_atom;
	ev.data.l[2] = _owner;
	ev.data.l[3] = 0;
	ev.data.l[4] = 0;

	X11::sendEvent(X11::getRoot(), False, StructureNotifyMask,
		       reinterpret_cast<XEvent*>(&ev));
}

/**
 * Handle DestroyNotify, removing client if any of the docked clients.
 */
bool
SystrayWidget::handleDestroyNotify(XDestroyWindowEvent* ev)
{
	SystrayWidget::Client* client = findClient(ev->window);
	if (client == nullptr) {
		return false;
	}
	removeTrayIcon(client, false);
	sendRequiredSizeChanged();

	return true;
}

/**
 * Handle ClientMessage if the message type is NET_SYSTEM_TRAY_OPCODE
 */
bool
SystrayWidget::handleClientMessage(XClientMessageEvent* ev)
{
	if (ev->message_type != X11::getAtom(NET_SYSTEM_TRAY_OPCODE)) {
		return false;
	}
	return handleSystemTrayOpcode(ev->data.l[1], ev->data.l[2]);
}

/**
 * Handle PropertyEvent if the property is XEMBED_INFO
 */
bool
SystrayWidget::handlePropertyNotify(XPropertyEvent* ev)
{
	Client* client;
	if (ev->atom != X11::getAtom(XEMBED_INFO)
	    || (client = findClient(ev->window)) == nullptr) {
		return false;
	}

	const bool is_mapped = client->isMapped();
	readXEmbedInfo(client);
	if (is_mapped != client->isMapped()) {
		if (client->isMapped()) {
			P_TRACE("systray: unmapping client " <<
				client->getWindow());
			X11::unmapWindow(client->getWindow());
		} else {
			P_TRACE("systray: mapping client " <<
				client->getWindow());
			X11::mapRaised(client->getWindow());
		}
		sendRequiredSizeChanged();
	}

	return true;
}

bool
SystrayWidget::handleSystemTrayOpcode(long opcode, long win)
{
	switch (opcode) {
	case SYSTEM_TRAY_REQUEST_DOCK: {
		P_TRACE("systray: SYSTEM_TRAY_REQUEST_DOCK " << win);
		SystrayWidget::Client* client = addTrayIcon(win);
		notifyTrayIcon(client);
		return true;
	}
	case SYSTEM_TRAY_BEGIN_MESSAGE:
		P_TRACE("systray: SYSTEM_TRAY_BEGIN_MESSAGE " << win);
		return true;
	case SYSTEM_TRAY_CANCEL_MESSAGE:
		P_TRACE("systray: SYSTEM_TRAY_CANCEL_MESSAGE " << win);
		return true;
	default:
		P_DBG("systray: unknown _NET_SYSTEM_TRAY_OPCODE " << opcode);
		return false;
	}
}

void
SystrayWidget::readXEmbedInfo(Client* client)
{
	const Atom xembed_info = X11::getAtom(XEMBED_INFO);
	uchar *data;
	ulong actual;
	if (! X11::getProperty(client->getWindow(), xembed_info, xembed_info,
			       2, &data, &actual)) {
		P_WARN("systray: failed to read _XEMBED_INFO from "
		       << client->getWindow());
		return;
	}

	if (actual == 2) {
		long *ldata = reinterpret_cast<long*>(data);
		P_TRACE("systray: client " << client->getWindow()
			<< " xembed version " << ldata[0]
			<< " xembed flags " << ldata[1]);
		client->setXEmbedVersion(ldata[0]);
		client->setFlags(ldata[1]);
	} else {
		P_TRACE("systray: client " << client->getWindow()
			<< " _XEMBED_INFO invalid, setting defaults");
		client->setXEmbedVersion(0);
		client->setFlags(XEMBED_FLAG_MAPPED);
	}

	X11::free(data);
}
