//
// WORefMenu.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"

#include "Client.hh"

using std::string;
using std::wstring;

//! @brief WORefMenu constructor
//! @param theme Pointer to Theme
//! @param title Title of menu
//! @param name Name of menu
//! @param decor_name Name of decor, defaults to MENU
WORefMenu::WORefMenu(const std::wstring &title,
                     const std::string &name, const std::string &decor_name)
    : PMenu(title, name, decor_name), PWinObjReference(0),
      _title_base(title),
      _title_pre(L" ["), _title_post(L"]")
{
}

//! @brief WORefMenu destructor
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

//! @brief Sets the reference and updates the title
void
WORefMenu::setWORef(PWinObj *wo_ref)
{
    PWinObjReference::setWORef(wo_ref);

    wstring title(_title_base);

    // if of client type, add the clients named to the title
    if (wo_ref && (wo_ref->getType() == PWinObj::WO_CLIENT)) {
        Client *client = static_cast<Client*>(wo_ref);
        title += _title_pre + client->getTitle()->getVisible() + _title_post;
    }

    setTitle(title);
}
