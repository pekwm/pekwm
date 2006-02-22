//
// WORefMenu.hh for pekwm
// Copyright (C) 2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"

#include "PScreen.hh"
#include "Client.hh"

using std::string;

//! @brief WORefMenu constructor
//! @param scr Pointer to PScreen
//! @param theme Pointer to Theme
//! @param name Title of menu, defaults to beeing empty
WORefMenu::WORefMenu(PScreen *scr, Theme *theme, const std::string &name) : PMenu(scr->getDpy(), theme, name),
_wo_ref(NULL), _title_base(name),
_title_pre(" ["), _title_post("]")
{
}

//! @brief WORefMenu destructor
WORefMenu::~WORefMenu(void)
{
}

//! @brief Sets the reference and updates the title
void
WORefMenu::setWORef(PWinObj *wo)
{
	_wo_ref = wo;

	string title = _title_base;

	// if of client type, add the clients named to the title
	if ((_wo_ref != NULL) && (_wo_ref->getType() == PWinObj::WO_CLIENT)) {
		Client *client = static_cast<Client*>(wo);
		title += _title_pre + client->getTitle()->getVisible() + _title_post;
	}

	setTitle(title);
}

#endif // MENUS
