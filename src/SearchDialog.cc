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
  _ae.action_list.back().setAction(ACTION_GOTO_CLIENT);

  // Setup menu for displaying results
  _result_menu = new PMenu(_dpy, _theme, L"", "");
  _result_menu->reparent(this, borderLeft(), borderTop() + getTitleHeight() + _text_wo->getHeight());
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
 * Called whenever the buffer has change, update the clients displayed.
 */
void
SearchDialog::bufChanged(void)
{
  InputDialog::bufChanged();
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

    list<Client*> matches;
    list<Client*>::iterator it(Client::client_begin());
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

  unsigned int width, height;
  getInputSize(width, height);

  if (_result_menu->size()) {
      resizeChild(_text_wo->getWidth(), height + _result_menu->getHeight());
      XRaiseWindow(_dpy, _result_menu->getWindow());
  } else {
      resizeChild(_text_wo->getWidth(), height);
      XLowerWindow(_dpy, _result_menu->getWindow());
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
        _wo_ref = 0;
        bufClear();

        // Clear the menu and hide it.
        _result_menu->clear();
        _previous_search.clear();
        XLowerWindow(_dpy, _result_menu->getWindow());
    }
}
