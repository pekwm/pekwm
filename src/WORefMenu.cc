//
// WORefMenu.cc for pekwm
// Copyright (C) 2004-2024 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"

#include "Client.hh"

#include "tk/PWinObj.hh"

/**
 * WORefMenu constructor
 * @param theme Pointer to Theme
 * @param title Title of menu
 * @param name Name of menu
 * @param decor_name Name of decor, defaults to MENU
 */
WORefMenu::WORefMenu(const std::string &title,
		     const std::string &name, const std::string &decor_name)
	: PMenu(title, name, decor_name),
	  PWinObjReference(nullptr),
	  _title_base(title),
	  _title_pre(" ["),
	  _title_post("]")
{
}

WORefMenu::~WORefMenu(void)
{
}

/**
 * When notified, unmap all windows as window menu refers to object
 * being removed.
 */
void
WORefMenu::notify(Observable *observable, Observation *observation)
{
	PWinObjReference::notify(observable, observation);
	unmapAll();
}

/**
 * Sets the reference and updates the title
 */
void
WORefMenu::setWORef(PWinObj *wo_ref)
{
	PWinObjReference::setWORef(wo_ref);

	std::string title(_title_base);

	// if of client type, add the clients named to the title
	if (wo_ref && (wo_ref->getType() == PWinObj::WO_CLIENT)) {
		Client *client = static_cast<Client*>(wo_ref);
		title += _title_pre
			 + client->getTitle()->getVisible()
			 + _title_post;
	}

	setTitle(title);
}
