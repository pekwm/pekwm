//
// SearchDialog.cc for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SEARCHDIALOG_HH_
#define _PEKWM_SEARCHDIALOG_HH_

#include "config.h"

#include "pekwm.hh"

#include "InputDialog.hh"
#include "PMenu.hh"

#include <string>

/**
 * Search dialog providing a dialog for searching clients together
 * with a menu that shows clients matching the search.
 */
class SearchDialog : public InputDialog {
public:
    SearchDialog();
    virtual ~SearchDialog(void);

    virtual void unmapWindow(void);

protected:
    virtual ActionEvent *exec(void);

    virtual void bufChanged(void);

    virtual void histNext(void);
    virtual void histPrev(void);

    virtual void updateSize(const Geometry &head);

private:
    uint findClients(const std::string &search);

    PMenu *_result_menu; /**< Menu for displaying results. */
    std::string _previous_search; /**< Buffer with previous search string. */
};

#endif // _PEKWM_SEARCHDIALOG_HH_
