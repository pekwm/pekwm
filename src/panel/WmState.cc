//
// WmState.cc for pekwm
// Copyright (C) 2022-2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "WmState.hh"

/** empty string, used as default return value. */
static std::string _empty_string;

WmState::WmState(VarData& var_data)
	: _var_data(var_data),
	  _active_window(None),
	  _workspace(0)
{
}

WmState::~WmState(void)
{
	std::vector<ClientInfo*>::iterator it = _clients.begin();
	for (; it != _clients.end(); ++it) {
		delete *it;
	}
}

void
WmState::read(void)
{
	readActiveWorkspace();
	readActiveWindow();
	readClientList();
	readDesktopNames();
	readRootProperties();

	pekwm::observerMapping()->notifyObservers(this, nullptr);
}

const std::string&
WmState::getWorkspaceName(uint num) const
{
	if (num < _desktop_names.size()) {
		return _desktop_names[num];
	}
	return _empty_string;
}

ClientInfo*
WmState::findClientInfo(Window win) const
{
	return findClientInfo(win, _clients);
}

bool
WmState::handlePropertyNotify(XPropertyEvent *ev)
{
	bool updated = false;
	Observation *observation = nullptr;

	P_DBG("XPropertyEvent window: " << ev->window << " (root: "
	      << (ev->window == X11::getRoot()) << ") atom: " << ev->atom);

	if (ev->window == X11::getRoot()) {
		if (ev->atom == X11::getAtom(NET_CURRENT_DESKTOP)) {
			updated = readActiveWorkspace();
		} else if (ev->atom == X11::getAtom(NET_ACTIVE_WINDOW)) {
			updated = readActiveWindow();
		} else if (ev->atom == X11::getAtom(NET_CLIENT_LIST)) {
			updated = readClientList();
			if (updated) {
				observation = &_client_list_changed;
			}
		} else if (ev->atom == X11::getAtom(XROOTPMAP_ID)) {
			observation = &_xrootpmap_id_changed;
		} else if (ev->atom == X11::getAtom(PEKWM_THEME)) {
			observation = &_pekwm_theme_changed;
		} else {
			updated = readRootProperty(ev->atom);
		}
	} else {
		ClientInfo *client_info = findClientInfo(ev->window, _clients);
		if (client_info != nullptr) {
			updated = client_info->handlePropertyNotify(ev);
			if (updated) {
				observation = &_client_state_changed;
			}
		}
	}

	if (updated || observation) {
		pekwm::observerMapping()->notifyObservers(this, observation);
	}

	return updated;
}

ClientInfo*
WmState::findClientInfo(Window win,
			const std::vector<ClientInfo*> &clients) const
{
	std::vector<ClientInfo*>::const_iterator it = clients.begin();
	for (; it != clients.end(); ++it) {
		if ((*it)->getWindow() == win) {
			return *it;
		}
	}
	return nullptr;
}

ClientInfo*
WmState::popClientInfo(Window win, std::vector<ClientInfo*> &clients)
{
	std::vector<ClientInfo*>::iterator it = clients.begin();
	for (; it != clients.end(); ++it) {
		if ((*it)->getWindow() == win) {
			ClientInfo *client_info = *it;
			clients.erase(it);
			return client_info;
		}
	}
	return nullptr;
}

bool
WmState::readActiveWorkspace(void)
{
	Cardinal workspace;
	if (! X11::getCardinal(X11::getRoot(), NET_CURRENT_DESKTOP,
			       workspace)) {
		P_TRACE("failed to read _NET_CURRENT_DESKTOP, setting to 0");
		_workspace = 0;
		return false;
	}
	_workspace = workspace;
	return true;
}

bool
WmState::readActiveWindow(void)
{
	if (! X11::getWindow(X11::getRoot(),
			     NET_ACTIVE_WINDOW, _active_window)) {
		P_TRACE("failed to read _NET_ACTIVE_WINDOW, setting to None");
		_active_window = None;
		return false;
	}
	return true;
}

bool
WmState::readClientList(void)
{
	client_info_vector old_clients(_clients);
	_clients.clear();

	ulong actual;
	Window *windows;
	bool updated = false;
	if (X11::getProperty(X11::getRoot(),
			     X11::getAtom(NET_CLIENT_LIST),
			     XA_WINDOW, 0,
			     reinterpret_cast<uchar**>(&windows), &actual)) {
		P_TRACE("read _NET_CLIENT_LIST, " << actual << " windows");
		for (uint i = 0; i < actual; i++) {
			ClientInfo *client_info = popClientInfo(windows[i],
								old_clients);
			if (client_info == nullptr) {
				updated = true;
				_clients.push_back(new ClientInfo(windows[i]));
			} else {
				_clients.push_back(client_info);
			}
		}
		X11::free(windows);
	}

	client_info_it it = old_clients.begin();
	for (; it != old_clients.end(); ++it) {
		updated = true;
		delete *it;
	}

	return updated;
}

bool
WmState::readDesktopNames(void)
{
	_desktop_names.clear();

	uchar *data;
	ulong data_length;
	if (! X11::getProperty(X11::getRoot(), X11::getAtom(NET_DESKTOP_NAMES),
			       X11::getAtom(UTF8_STRING), 0,
			       &data, &data_length)) {
		return false;
	}

	char *name = reinterpret_cast<char*>(data);
	for (ulong i = 0; i < data_length; ) {
		_desktop_names.push_back(name);
		int name_len = strlen(name);
		i += name_len + 1;
		name += name_len + 1;
	}
	X11::free(data);

	return true;
}

void
WmState::readRootProperties(void)
{
	std::vector<Atom> atoms;
	if (! X11::listProperties(X11::getRoot(), atoms)) {
		return;
	}

	std::vector<Atom>::iterator it = atoms.begin();
	for (; it != atoms.end(); ++it) {
		readRootProperty(*it);
	}
}

bool
WmState::readRootProperty(Atom atom)
{
	bool res = false;
	const std::string& name = getAtomName(atom);
	if (name.size() > 0) {
		std::string value;
		if (X11::getStringId(X11::getRoot(), atom, value)) {
			_var_data.set("ATOM_" + name, value);
			res = true;
		}
	}
	return res;
}

const std::string&
WmState::getAtomName(Atom atom)
{
	std::map<Atom, std::string>::iterator it = _atom_names.find(atom);
	if (it != _atom_names.end()) {
		return it->second;
	}
	_atom_names[atom] = X11::getAtomIdString(atom);
	return _atom_names[atom];
}


