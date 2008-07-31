//
// SearchDialog.cc for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "SearchDialog.hh"

#include "Client.hh"
#include "RegexString.hh"

#include <iostream>
#include <list>

using std::cerr;
using std::endl;
using std::list;
using std::wcerr;

/**
 * SearchDialog constructor.
 */
SearchDialog::SearchDialog(Display *dpy, Theme *theme)
  : InputDialog(dpy, theme, L"Search")
{
    _type = PWinObj::WO_SEARCH_DIALOG;
}

/**
 * Called whenever the buffer has change, update the clients displayed.
 */
void
SearchDialog::bufChanged(void)
{
    InputDialog::bufChanged();

    // FIXME: Update list of clients
    findClients(_buf);
}

/**
 * Run when INPUT_EXEC is entered in the dialog giving the selected
 * Client focus if any.
 */
ActionEvent*
SearchDialog::exec(void)
{
    // FIXME: Implement exec

    return 0;
}

/**
 * Search list of clients for matching titles.
 *
 * @param search Regexp to search, case insensitive
 * @return Number of matches
 */
uint
SearchDialog::findClients(const std::wstring &search)
{
    // FIXME: Clear previous matches

    if (search.size() > 0) {
        RegexString search_re(search);
        if (! search_re.is_match_ok()) {
            return 0;
        }

        list<Client*> matches;
        list<Client*>::iterator it(Client::client_begin());
        for (; it != Client::client_end(); ++it) {
            if (search_re == (*it)->getTitle()->getReal()) {
                matches.push_back(*it);
            }
        }
    }

    return 0;
}
