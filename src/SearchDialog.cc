//
// SearchDialog.cc for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

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
SearchDialog::SearchDialog(Theme *theme)
  : InputDialog(theme, L"Search"),
    _result_menu(0)
{
    _type = PWinObj::WO_SEARCH_DIALOG;

    // Set up ActionEvent
    _ae.action_list.back().setAction(ACTION_GOTO_CLIENT);

    // Set up menu for displaying results
    _result_menu = new PMenu(_theme, L"", "");
    _result_menu->reparent(this, borderLeft(), borderTop() + getTitleHeight() + _text_wo->getHeight());
    _result_menu->setLayer(LAYER_DESKTOP); // Ignore when placing
    _result_menu->setSticky(STATE_SET);
    _result_menu->setBorder(STATE_UNSET);
    _result_menu->setTitlebar(STATE_UNSET);
    _result_menu->setFocusable(false);
    _result_menu->mapWindow();
}

/**
 * SearchDialog destructor.
 */
SearchDialog::~SearchDialog(void)
{
    delete _result_menu;
}

/**
 * Run when INPUT_EXEC is entered in the dialog giving the selected
 * Client focus if any.
 */
ActionEvent*
SearchDialog::exec(void)
{
    // InputDialog::close() may have overwritten our action.
    _ae.action_list.back().setAction(ACTION_GOTO_CLIENT);

    PWinObj *wo_ref = 0;
    if (_result_menu->getItemCurr()) {
        wo_ref = _result_menu->getItemCurr()->getWORef();
    }
    setWORef(wo_ref);

    return &_ae;
}

/**
 * Called whenever the buffer has changed. Updates the displayed clients.
 */
void
SearchDialog::bufChanged(void)
{
    InputDialog::bufChanged();
    findClients(_buf);
}

/**
 * Focus next item in result menu.
 */
void
SearchDialog::histNext(void)
{
    _result_menu->selectItemRel(1);
}

/**
 * Focus previous item in result menu.
 */
void
SearchDialog::histPrev(void)
{
    _result_menu->selectItemRel(-1);
}

/**
 * Update size making sure result menu fits.
 */
void
SearchDialog::updateSize(const Geometry &head)
{
    InputDialog::updateSize(head);
    _result_menu->setMenuWidth(_text_wo->getWidth());
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
    // Do nothing if search has not changed.
    if (_previous_search == search) {
        return _result_menu->size();
    }
    _previous_search = search;

    _result_menu->removeAll();
    if (search.size() > 0) {
        RegexString search_re(L"/" + search + L"/i");
        if (! search_re.is_match_ok()) {
            return 0;
        }

        vector<Client*> matches;
        vector<Client*>::const_iterator it(Client::client_begin());
        for (; it != Client::client_end(); ++it) {
            if ((*it)->isFocusable()  && ! (*it)->isSkip(SKIP_FOCUS_TOGGLE)
                 && search_re == (*it)->getTitle()->getReal()) {
                matches.push_back(*it);
            }
        }

        for (it = matches.begin(); it != matches.end(); ++it) {
            _result_menu->insert((*it)->getTitle()->getVisible(), *it, (*it)->getIcon());
        }
    }

    // Rebuild menu and make room for it
    _result_menu->buildMenu();

    Geometry head;
    X11::getHeadInfo(getHead(), head);

    unsigned int width, height;
    getInputSize(head, width, height);

    if (_result_menu->size()) {
        resizeChild(_text_wo->getWidth(), height + _result_menu->getHeight());
        XRaiseWindow(X11::getDpy(), _result_menu->getWindow());
        // Render first item as selected, needs to be done after map/raise.
        _result_menu->selectItemNum(0);
    } else {
        resizeChild(_text_wo->getWidth(), height);
        XLowerWindow(X11::getDpy(), _result_menu->getWindow());
    }

    return 0;
}

/**
 * Unmap window and clear buffer, result menu and window reference.
 */
void
SearchDialog::unmapWindow(void)
{
    if (_mapped) {
        InputDialog::unmapWindow();
        setWORef(0);
        bufClear();

        // Clear the menu and hide it.
        _result_menu->clear();
        _previous_search.clear();
        XLowerWindow(X11::getDpy(), _result_menu->getWindow());
    }
}
