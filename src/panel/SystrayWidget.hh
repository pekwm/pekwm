//
// SystrayWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_SYSTRAY_WIDGET_HH_
#define _PEKWM_PANEL_SYSTRAY_WIDGET_HH_

#include <vector>

#include "pekwm_panel.hh"
#include "CfgParser.hh"
#include "PanelWidget.hh"

#include "../tk/PWinObj.hh"

/**
 * Widget providing a systray area, only one widget is allowed in the panel
 * configuration.
 */
class SystrayWidget : public PanelWidget,
		      public Observable {
public:
	class Client : public PWinObj {
	public:
		Client(Window win);
		virtual ~Client();

		long getXEmbedVersion() const { return _xembed_version; }
		void setXEmbedVersion(long v) { _xembed_version = v; }

		bool isMapped() const;
		void setFlags(long f) { _xembed_flags = f; }

	private:
		long _xembed_version;
		long _xembed_flags;
	};
	typedef std::vector<Client*> client_vector;
	typedef client_vector::iterator client_it;
	typedef client_vector::const_iterator client_cit;

	SystrayWidget(const PWinObj* parent,
		      Observer* observer,
		      const PanelTheme& theme,
		      const SizeReq& size_req,
		      const CfgParser::Entry* section);
	virtual ~SystrayWidget();

	virtual uint getRequiredSize(void) const;
	virtual void move(int x);
	virtual void click(int, int);
	virtual void render(Render& rend);

	virtual bool operator==(Window win) const {
		if (win == _owner || win == _parent->getWindow()) {
			return true;
		}
		return findClient(win) != nullptr;
	}
	virtual bool handleXEvent(XEvent* ev);

private:
	SystrayWidget::Client* addTrayIcon(Window win);
	void notifyTrayIcon(SystrayWidget::Client* client);
	void removeTrayIcon(SystrayWidget::Client* client,
			    bool reparent);
	SystrayWidget::Client* findClient(Window win) const;
	int numClientsMapped() const;

	bool claimNetSystray();
	void notifyClaimedSystray();

	bool handleDestroyNotify(XDestroyWindowEvent* ev);
	bool handleClientMessage(XClientMessageEvent* ev);
	bool handlePropertyNotify(XPropertyEvent* ev);

	bool handleSystemTrayOpcode(long opcode, long win);
	void readXEmbedInfo(Client* client);

	void sendRequiredSizeChanged();

private:
	/** Observer for requrired size notifications */
	Observer* _observer;
	/** Owner window for _NET_SYSTEM_TRAY_Sn */
	Window _owner;
	/** _NET_SYSTEM_TRAY_Sn atom */
	Atom _owner_atom;

	/** Clients in the systray */
	client_vector _clients;
};

#endif // _PEKWM_PANEL_SYSTRAY_WIDGET_HH_
