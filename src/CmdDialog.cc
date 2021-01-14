//
// CmdDialog.cc for pekwm
// Copyright (C) 2004-2015 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>
#include <list>
#include <cwctype>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h> // XLookupString
#include <X11/keysym.h>
}

#include "PWinObj.hh"
#include "CmdDialog.hh"
#include "Config.hh"

/**
 * CmdDialog constructor, init and load history file.
 *
 * @todo Make size configurable.
 */
CmdDialog::CmdDialog()
  : InputDialog(L"Enter command"), _exec_count(0)
{
    _type = PWinObj::WO_CMD_DIALOG;

    if (Config::instance()->getCmdDialogHistoryFile().size() > 0) {
        loadHistory(Config::instance()->getCmdDialogHistoryFile());
    }
}

/**
 * CmdDialog de-structor, clean up and save history file.
 */
CmdDialog::~CmdDialog(void)
{
    if (Config::instance()->getCmdDialogHistoryFile().size() > 0) {
        saveHistory(Config::instance()->getCmdDialogHistoryFile());
    }
}


//! @brief Parses _buf and tries to generate an ActionEvent
//! @return Pointer to ActionEvent.
ActionEvent*
CmdDialog::exec(void)
{
    // Update history
    if (Config::instance()->isCmdDialogHistoryUnique()) {
        addHistoryUnique(_buf);
    } else {
        _history.push_back(_buf);
    }
    if (_history.size() > static_cast<uint>(Config::instance()->getCmdDialogHistorySize())) {
        _history.erase(_history.begin());
    }

    // Persist changes
    if (Config::instance()->getCmdDialogHistorySaveInterval() > 0
        && Config::instance()->getCmdDialogHistoryFile().size() > 0
        && ++_exec_count > Config::instance()->getCmdDialogHistorySaveInterval()) {
        saveHistory(Config::instance()->getCmdDialogHistoryFile());
        _exec_count = 0;
    }
    
    // Check if it's a valid Action, if not we assume it's a command and try
    // to execute it.
    auto buf_mb(Util::to_mb_str(_buf));
    if (! Config::instance()->parseAction(buf_mb, _ae.action_list.back(), KEYGRABBER_OK)) {
        _ae.action_list.back().setAction(ACTION_EXEC);
        _ae.action_list.back().setParamS(buf_mb);
    }

    return &_ae;
}

/**
 * Map window, overloaded to refresh completions.
 */
void
CmdDialog::mapWindow(void)
{
    InputDialog::mapWindow();
    _completer.refresh();
}

/**
 * Unmaps window, overloaded to clear buffer and unset reference.
 */
void
CmdDialog::unmapWindow(void)
{
    if (_mapped) {
        InputDialog::unmapWindow();
        setWORef(0);
        bufClear();
        _completer.clear();
    }
}

/**
 * Tab completion, complete word at cursor position.
 */
void
CmdDialog::complete(void)
{
    // Find completion if changed since last time.
    if (_buf != _buf_on_complete_result) {
        InputDialog::complete();
        _complete_list = _completer.find_completions(_buf, _pos);
        _complete_it = _complete_list.begin();
    }

    if (_complete_list.size()) {
        _buf = _completer.do_complete(_buf_on_complete, _pos, _complete_list, _complete_it);
        _buf_on_complete_result = _buf;
    }
}

/**
 * Map CmdDialog centered on wo_ref is specified, else _wo_ref.
 */
void
CmdDialog::mapCentered(const std::string &buf, const Geometry &geom, PWinObj *wo_ref)
{
    if (wo_ref != 0) {
        setWORef(wo_ref);
    }
    InputDialog::mapCentered(buf, geom, wo_ref);
}
