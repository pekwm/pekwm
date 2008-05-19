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

#include <string>

/**
 * Search dialog providing a dialog for searching clients together
 * with a menu that shows clients matching the search.
 */
class SearchDialog : public InputDialog {
public:
  SearchDialog(Display *dpy, Theme *theme);

protected:
  virtual ActionEvent *exec(void);

  virtual void bufChanged(void);

private:
  uint findClients(const std::wstring &search);
};

#endif // _SEARCH_DIALOG_HH_
