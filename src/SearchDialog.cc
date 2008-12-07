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
  : InputDialog(dpy, theme, L"Search"),
    _result_menu(0)
{
  _type = PWinObj::WO_SEARCH_DIALOG;

  // Setup ActionEvent
  _ae.action_list.back().setAction(ACTION_FOCUS);

  // Setup menu for displaying results
  _result_menu = new PMenu(_dpy, _theme, L"", "");
  _result_menu->reparent(this, borderLeft(), borderTop() + getTitleHeight() + _text_wo->getHeight());
  _result_menu->setSticky(STATE_SET);
  _result_menu->setBorder(STATE_UNSET);
  _result_menu->setTitlebar(STATE_UNSET);
  _result_menu->mapWindow();
}

SearchDialog::~SearchDialog(void) 
{
    delete _result_menu;
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
  if (_result_menu->getItemCurr()) {
    _wo_ref = _result_menu->getItemCurr()->getWORef();
  } else {
    _wo_ref = 0;
  }

  return &_ae;
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
SearchDialog::updateSize(void)
{
  InputDialog::updateSize();

  if (_result_menu->size()) {
    _result_menu->setMenuWidth(_text_wo->getWidth());
    resize(_gm.width, _gm.height + _result_menu->getHeight());
  }
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
  _result_menu->removeAll();

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

    for (it = matches.begin(); it != matches.end(); ++it) {
      _result_menu->insert((*it)->getTitle()->getVisible(), *it, (*it)->getIcon());
    }
  }

  _result_menu->buildMenu();
  updateSize();

  return 0;
}
