//
// workspaces.hh for pekwm
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

#ifndef _WORKSPACES_HH_
#define _WORKSPACES_HH_

#include "atoms.hh"

#include <string>
#include <vector>
#include <list>
#include <algorithm>

#include <X11/Xlib.h>

class Client;

class Workspaces
{
public:
	class Workspace {
	public:
		Workspace(Display *d, unsigned int n, const std::string &name)
			: dpy(d), m_num(n), m_name(name), m_last_focused_client(NULL) { }
		~Workspace() { }

		inline std::list<Client*> *getClientList(void) {
			return &m_client_list; }

		inline void setNum(unsigned int n) { m_num = n; }
		inline void setName(const std::string &name) { m_name = name; }

		void activate(void);
		void deactivate(void);

		inline void addClient(Client *c) { if (c) m_client_list.push_back(c); }
		inline void removeClient(Client *c) { if (c) m_client_list.remove(c); }
		inline void setLastFocusedClient(Client *c) { m_last_focused_client = c; }

		void raiseClient(Client *c);
		void lowerClient(Client *c);
		void stackClientAbove(Client *c, Window win, bool restack = true);
		void stackClientBelow(Client *c, Window win, bool restack = true);

	private:
		void stackWinUnderWin(Window win_over, Window win_under);


	private:
		Display *dpy;

		unsigned int m_num;
		std::string m_name; // if we are going to support workspace names
		
		std::list<Client*> m_client_list;
		Client *m_last_focused_client;
	};

	Workspaces(Display *d, EwmhAtoms *ewmh, unsigned int num);
	~Workspaces();

	inline std::list<Client*> *getClientList(unsigned int workspace) {
		if (workspace >= m_num_workspaces)
			return NULL;
		else
			return m_workspace_list[workspace]->getClientList();
	}
	inline std::list<Client*> *getStickyClientList(void) {
		return &m_sticky_client_list;
	}

	inline unsigned int getNumWorkspaces(void) const { return m_num_workspaces; }
	void setNumWorkspaces(unsigned int num_workspaces);
	inline void setReportOnlyFrames(bool r) { m_report_only_frames = r; }

	void activate(unsigned int workspace);

	Client* getTopClient(unsigned int workspace);

	inline void addClient(Client *c, unsigned int workspace) {
		if (workspace < m_num_workspaces)
			m_workspace_list[workspace]->addClient(c);
	}
	inline void removeClient(Client *c, unsigned int workspace) {
		if (workspace < m_num_workspaces)
			m_workspace_list[workspace]->removeClient(c);
	}

	inline void setLastFocusedClient(Client *c, unsigned int workspace) {
		if (workspace < m_num_workspaces)
			m_workspace_list[workspace]->setLastFocusedClient(c);
	}

	inline void stickClient(Client *c) {
		if (c) m_sticky_client_list.push_back(c); }
	inline void unstickClient(Client *c) {
		if (c) m_sticky_client_list.remove(c); }

	inline void raiseClient(Client *c, unsigned int workspace) {
		if (workspace < m_num_workspaces) {
			m_workspace_list[workspace]->raiseClient(c);
			updateClientList();
		}
	}
	inline void lowerClient(Client *c, unsigned int workspace) {
		if (workspace < m_num_workspaces) {
			m_workspace_list[workspace]->lowerClient(c);
			updateClientList();
		}
	}
	inline void stackClientAbove(Client *c, Window win, unsigned int workspace, bool restack = false) {
		if (workspace < m_num_workspaces) {
			m_workspace_list[workspace]->stackClientAbove(c, win, restack);
			updateClientList();
		}
	}
	inline void stackClientBelow(Client *c, Window win, unsigned int workspace, bool restack = false) {
		if (workspace < m_num_workspaces) {
			m_workspace_list[workspace]->stackClientBelow(c, win, restack);
			updateClientList();
		}
	}

	void updateClientList(void);

private:
	Display *dpy;
	EwmhAtoms *ewmh_atoms;

	unsigned int m_num_workspaces;
	unsigned int m_active_workspace;

	bool m_report_only_frames;

	std::vector<Workspace*> m_workspace_list;
	std::list<Client*> m_sticky_client_list;
};

#endif // _WORKSPACES_HH_
