//
// ActionMenu.hh for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_ACTIONMENU_HH_
#define _PEKWM_ACTIONMENU_HH_

#include "config.h"

#include "pekwm.hh"
#include "Action.hh" // For ActionOk
#include "CfgParser.hh"
#include "WORefMenu.hh"

#include <string>

class ActionHandler;

class ActionMenu : public WORefMenu
{
public:
	ActionMenu(MenuType type, ActionHandler *act,
		   const std::string &title, const std::string &name,
		   const std::string &decor_name = "MENU");
	virtual ~ActionMenu(void);

	// START - PWinObj interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	// END - PWinObj interface.

	virtual void handleItemExec(PMenu::Item *item);

	virtual void insert(PMenu::Item *item);
	virtual void insert(std::vector<PMenu::Item*>::iterator at,
			    PMenu::Item *item);
	virtual void insert(const std::string &name, PWinObj *wo_ref = 0,
			    PTexture *icon = 0);
	virtual void insert(const std::string &name, const ActionEvent &ae,
			    PWinObj *wo_ref = 0, PTexture *icon = 0);

	virtual void reload(CfgParser::Entry *section);

	virtual void remove(PMenu::Item *item);
	virtual void removeAll(void);

protected:
	void rebuildDynamic(void);
	void removeDynamic(void);

	virtual CfgParser::Entry* runDynamic(CfgParser& parser,
					     const std::string& src);

private:
	void parse(CfgParser::Entry *section, PMenu::Item *parent=0);
	PTexture *getIcon(CfgParser::Entry *value);

private:
	ActionHandler *_act;

	ActionOk _action_ok;
	std::vector<PMenu::Item*>::size_type _insert_at;

	/** Set to true if any of the entries in the menu is dynamic. */
	bool _has_dynamic;
};

#endif // _PEKWM_ACTIONMENU_HH_
