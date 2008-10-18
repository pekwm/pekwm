//
// WORefMenu.hh for pekwm
// Copyright © 2004-2007 Claes Nästen <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef MENUS

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"

#include "PScreen.hh"
#include "Client.hh"

using std::string;
using std::wstring;

//! @brief WORefMenu constructor
//! @param scr Pointer to PScreen
//! @param theme Pointer to Theme
//! @param title Title of menu
//! @param name Name of menu
//! @param decor_name Name of decor, defaults to MENU
WORefMenu::WORefMenu(PScreen *scr, Theme *theme, const std::wstring &title,
                     const std::string &name, const std::string &decor_name)
    : PMenu(scr->getDpy(), theme, title, name, decor_name),
      _wo_ref(0), _title_base(title),
      _title_pre(L" ["), _title_post(L"]")
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

    wstring title(_title_base);

    // if of client type, add the clients named to the title
    if (_wo_ref && (_wo_ref->getType() == PWinObj::WO_CLIENT)) {
        Client *client = static_cast<Client*>(wo);
        title += _title_pre + client->getTitle()->getVisible() + _title_post;
    }

    setTitle(title);
}

#endif // MENUS
