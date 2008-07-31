//
// CmdDialog.cc for pekwm
// Copyright © 2004-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>
#include <list>
#include <cwctype>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h> // XLookupString
#include <X11/keysym.h>
}

#include "PWinObj.hh"
#include "PDecor.hh"
#include "CmdDialog.hh"
#include "Config.hh"
#include "PScreen.hh"
#include "PixmapHandler.hh"
#include "KeyGrabber.hh"
#include "ScreenResources.hh"
#include "Workspaces.hh"

using std::cerr;
using std::endl;
using std::list;
using std::string;
using std::wstring;

//! @brief CmdDialog constructor
//! @todo Initial size, configurable?
CmdDialog::CmdDialog(Display *dpy, Theme *theme)
  : InputDialog(dpy, theme, L"Enter command"),
    _wo_ref(0)
{
    _type = PWinObj::WO_CMD_DIALOG;
}

//! @brief Parses _buf and tries to generate an ActionEvent
//! @return Pointer to ActionEvent.
ActionEvent*
CmdDialog::exec(void)
{
    _hist_list.push_back(_buf);
    if (_hist_list.size() > 10) { // FIXME: make configurable
        _hist_list.pop_front();
    }
    
    // Check if it's a valid Action, if not we assume it's a command and try
    // to execute it.
    string buf_mb(Util::to_mb_str(_buf));
    if (! Config::instance()->parseAction(buf_mb, _ae.action_list.back(),
                                         KEYGRABBER_OK)) {
        _ae.action_list.back().setAction(ACTION_EXEC);
        _ae.action_list.back().setParamS(buf_mb);
    }

    return &_ae;
}

//! @brief Unmaps window, overloaded to clear buffer.
void
CmdDialog::unmapWindow(void)
{
    if (_mapped) {
        PDecor::unmapWindow();

        _wo_ref = NULL;
        bufClear();
    }
}

//! @brief Tab completion, complete word at cursor position.
void
CmdDialog::complete(void)
{
}

void
CmdDialog::mapCentered(const std::string &buf, bool focus, PWinObj *wo_ref)
{
    _wo_ref = wo_ref ? wo_ref : _wo_ref;

    InputDialog::mapCentered(buf, focus, wo_ref);
}
