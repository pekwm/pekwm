//
// SearchDialog.cc for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _SEARCH_DIALOG_HH_
#define _SEARCH_DIALOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include "InputDialog.hh"
#include "Theme.hh"
#include "PMenu.hh"

#include <string>

/**
 * Search dialog providing a dialog for searching clients together
 * with a menu that shows clients matching the search.
 */
class SearchDialog : public InputDialog {
public:
  SearchDialog(Display *dpy, Theme *theme);

protected:
  virtual void bufChanged(void);

  virtual ActionEvent *exec(void);

  virtual void histNext(void);
  virtual void histPrev(void);

  virtual void updateSize(void);

private:
  uint findClients(const std::wstring &search);

  PMenu *_result_menu; /**< Menu for displaying results. */
};

#endif // _SEARCH_DIALOG_HH_
