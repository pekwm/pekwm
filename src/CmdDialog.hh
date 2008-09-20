//
// CmdDialog.hh for pekwm
// Copyright © 2004-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CMD_DIALOG_HH_
#define _CMD_DIALOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Action.hh"
#include "InputDialog.hh"
#include "Theme.hh"

#include <list>
#include <string>

//! @brief CmdDialog presenting the user with an pekwm action command line.
class CmdDialog : public InputDialog
{
public:
    CmdDialog(Display *dpy, Theme *theme);
    virtual ~CmdDialog(void);

    void unmapWindow(void);

    //! @brief Returns the PWinObj the CmdDialog executes actions on.
    inline PWinObj *getWORef(void) { return _wo_ref; }
    //! @brief Sets the PWinObj the CmdDialog executes actions on.
    inline void setWORef(PWinObj *wo) { _wo_ref = wo; }

    virtual void mapCentered(const std::string &buf, bool focus, PWinObj *wo_ref);

private:
    void render(void);

    virtual ActionEvent *exec(void);
    virtual void complete(void);

private:
  PWinObj *_wo_ref;
  int _exec_count; /**< Number of CmdDialog has run exec since last history save. */
};

#endif // _CMD_DIALOG_HH_
