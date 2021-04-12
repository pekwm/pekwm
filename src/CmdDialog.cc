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

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h> // XLookupString
#include <X11/keysym.h>
}

#include "Charset.hh"
#include "PWinObj.hh"
#include "CmdDialog.hh"
#include "Config.hh"

/**
 * CmdDialog constructor, init and load history file.
 *
 * @todo Make size configurable.
 */
CmdDialog::CmdDialog()
  : InputDialog("Enter command"), _exec_count(0)
{
    _type = PWinObj::WO_CMD_DIALOG;

    if (pekwm::config()->getCmdDialogHistoryFile().size() > 0) {
        loadHistory(pekwm::config()->getCmdDialogHistoryFile());
    }
}

/**
 * CmdDialog de-structor, clean up and save history file.
 */
CmdDialog::~CmdDialog(void)
{
    if (pekwm::config()->getCmdDialogHistoryFile().size() > 0) {
        saveHistory(pekwm::config()->getCmdDialogHistoryFile());
    }
}

/**
 * Parses _buf and tries to generate an ActionEvent
 * @return Pointer to ActionEvent.
 */
ActionEvent*
CmdDialog::exec(void)
{
    auto cfg = pekwm::config();

    // Update history
    addHistory(str(),
               cfg->isCmdDialogHistoryUnique(),
               static_cast<uint>(cfg->getCmdDialogHistorySize()));

    // Persist changes
    if (cfg->getCmdDialogHistorySaveInterval() > 0
        && cfg->getCmdDialogHistoryFile().size() > 0
        && ++_exec_count > cfg->getCmdDialogHistorySaveInterval()) {
        saveHistory(cfg->getCmdDialogHistoryFile());
        _exec_count = 0;
    }

    // Check if it is a valid Action, if not assume it is a shell
    // command and try to execute it.
    if (! ActionConfig::parseAction(str(), ae().action_list.back(),
                                    KEYGRABBER_OK)) {
        ae().action_list.back().setAction(ACTION_EXEC);
        ae().action_list.back().setParamS(str());
    }

    return &(ae());
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
        buf().clear();
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
    if (str() != _buf_on_complete_result) {
        InputDialog::complete();
        _complete_list = _completer.find_completions(str(), buf().pos());
        _complete_it = _complete_list.begin();
    }

    if (_complete_list.size()) {
        uint pos = buf().pos();
        auto val = _completer.do_complete(_buf_on_complete, pos,
                                          _complete_list, _complete_it);
        buf().setBuf(val);
        buf().setPos(pos);
        _buf_on_complete_result = val;
    }
}

/**
 * Restore buffer and clear completion buffers.
 */
void
CmdDialog::completeAbort(void)
{
    if (_buf_on_complete.size()) {
        buf().setBuf(_buf_on_complete);
        buf().setPos(_pos_on_complete);
    }

    completeReset();
}

/**
 * Clear the completion buffer.
 */
void
CmdDialog::completeReset(void)
{
    _buf_on_complete = "";
    _buf_on_complete_result = "";
    _pos_on_complete = 0;
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
