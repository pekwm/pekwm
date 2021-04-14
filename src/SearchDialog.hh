//
// SearchDialog.cc for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

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

    virtual void unmapWindow(void) override;

protected:
    virtual ActionEvent *exec(void) override;

    virtual void bufChanged(void) override;

    virtual void histNext(void) override;
    virtual void histPrev(void) override;

    virtual void updateSize(const Geometry &head) override;

private:
    uint findClients(const std::string &search);

    PMenu *_result_menu; /**< Menu for displaying results. */
    std::string _previous_search; /**< Buffer with previous search string. */
};
