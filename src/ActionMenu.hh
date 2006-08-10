//
// ActionMenu.hh for pekwm
// Copyright (C) 2002-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef MENUS

#ifndef _ACTIONMENU_HH_
#define _ACTIONMENU_HH_

#include "pekwm.hh"
#include "Action.hh" // For ActionOk
#include "CfgParser.hh"

#include <string>
#include <list>

class WORefMenu;
class PScreen;
class Theme;
class ActionHandler;
class WindowManager;

class PMenu::Item;

class ActionMenu : public WORefMenu
{
public:
	ActionMenu(WindowManager *wm, MenuType type,
						 const std::string &title, const std::string &name,
						 const std::string &decor_name = "MENU");
	virtual ~ActionMenu(void);

	// START - PWinObj interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	// END - PWinObj interface.

	virtual void handleItemExec(PMenu::Item *item);

  virtual void insert(PMenu::Item *item);
  virtual void insert(const std::string &or_name, PWinObj *op_wo_ref = NULL);
  virtual void insert(const std::string &or_name, const ActionEvent &or_ae,
                      PWinObj *op_wo_ref = NULL);

	virtual void reload(CfgParser::Entry *section);

	virtual void remove(PMenu::Item *item);
	virtual void removeAll(void);

private:
	void parse(CfgParser::Entry *op_section, bool dynamic = false);

	void rebuildDynamic(void);
	void removeDynamic(void);

private:
	WindowManager *_wm;
	ActionHandler *_act;

	ActionOk _action_ok;
	std::list<PMenu::Item*>::iterator _insert_at;

	bool _has_dynamic;
};

#endif // _ACTIONMENU_HH_

#endif // MENUS
