//
// WmState.hh for pekwm
// Copyright (C) 2022-2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_WM_STATE_HH_
#define _PEKWM_PANEL_WM_STATE_HH_

#include "pekwm_panel.hh"
#include "ClientInfo.hh"
#include "Observable.hh"
#include "VarData.hh"
#include "X11.hh"

/**
 * Current window manager state.
 */
class WmState : public Observable
{
public:
	class XROOTPMAP_ID_Changed : public Observation {
	};

	/**
	 * Observation sent whenever the client list of the WM state has been
	 * updated.
	 */
	class ClientList_Changed : public Observation {
	};

	/**
	 * Observation sent whenever a single client state from the client list
	 * has been updated.
	 */
	class ClientState_Changed : public Observation {
	};

	typedef std::vector<ClientInfo*> client_info_vector;
	typedef client_info_vector::const_iterator client_info_it;

	WmState(VarData& var_data);
	virtual ~WmState(void);

	void read(void);

	uint getActiveWorkspace(void) const { return _workspace; }
	const std::string& getWorkspaceName(uint num) const;
	Window getActiveWindow(void) const { return _active_window; }
	ClientInfo *findClientInfo(Window win) const;

	uint numClients(void) const { return _clients.size(); }
	client_info_it clientsBegin(void) const { return _clients.begin(); }
	client_info_it clientsEnd(void) const { return _clients.end(); }

	bool handlePropertyNotify(XPropertyEvent *ev);

private:
	ClientInfo* findClientInfo(Window win,
				   const std::vector<ClientInfo*> &clients)
		const;
	ClientInfo* popClientInfo(Window win,
				  std::vector<ClientInfo*> &clients);

	bool readActiveWorkspace(void);
	bool readActiveWindow(void);
	bool readClientList(void);
	bool readDesktopNames(void);
	void readRootProperties(void);
	bool readRootProperty(Atom atom);

	const std::string& getAtomName(Atom atom);

private:
	VarData& _var_data;
	Window _active_window;
	uint _workspace;
	client_info_vector _clients;
	std::vector<std::string> _desktop_names;
	std::map<Atom, std::string> _atom_names;

	XROOTPMAP_ID_Changed _xrootpmap_id_changed;
	ClientList_Changed _client_list_changed;
	ClientState_Changed _client_state_changed;
};

#endif // _PEKWM_PANEL_WM_STATE_HH_
