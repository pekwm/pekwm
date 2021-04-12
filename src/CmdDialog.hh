//
// CmdDialog.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "Action.hh"
#include "Completer.hh"
#include "InputDialog.hh"

#include <list>
#include <string>

//! @brief CmdDialog presenting the user with an pekwm action command line.
class CmdDialog : public InputDialog
{
public:
    CmdDialog();
    virtual ~CmdDialog(void);

    // BEGIN - PWinObj interface
    virtual void mapWindow(void);
    // END - PWinObj interface
    void unmapWindow(void);

    virtual void mapCentered(const std::string &buf, const Geometry &geom,
                             PWinObj *wo_ref);

private:
    void render(void);

    virtual ActionEvent *exec(void) override;
    virtual void complete(void) override;
    virtual void completeAbort(void) override;
    virtual void completeReset(void) override;

private:
    Completer _completer; /**< Completer used completing actions. */
    /** List of completions found by completer. */
    complete_list _complete_list;
    complete_it _complete_it; /**< Iterator used to step between completions. */

    std::string _buf_on_complete; /**< Buffer before completion. */
    std::string _buf_on_complete_result; /** Buffer after completion. */
    unsigned int _pos_on_complete; /**< Cursor position on completion start. */

    /** Number of CmdDialog has run exec since last history save. */
    int _exec_count;
};
