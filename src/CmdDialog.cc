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

/**
 * CmdDialog constructor, init and load history file.
 *
 * @todo Make size configurable.
 */
CmdDialog::CmdDialog(Display *dpy, Theme *theme)
  : InputDialog(dpy, theme, L"Enter command"),
    _completer(L";"), _exec_count(0)
{
  _type = PWinObj::WO_CMD_DIALOG;

  // Setup completer
  _completer.add_method(new PathCompleterMethod());

  if (Config::instance()->getCmdDialogHistoryFile().size() > 0) {
    _hist_list.load(Config::instance()->getCmdDialogHistoryFile());
  }
}

/**
 * CmdDialog de-structor, clean up and save history file.
 */
CmdDialog::~CmdDialog(void)
{
  if (Config::instance()->getCmdDialogHistoryFile().size() > 0) {
    _hist_list.save(Config::instance()->getCmdDialogHistoryFile());
  }
}


//! @brief Parses _buf and tries to generate an ActionEvent
//! @return Pointer to ActionEvent.
ActionEvent*
CmdDialog::exec(void)
{
  // Update history
  if (Config::instance()->isCmdDialogHistoryUnique()) {
    _hist_list.push_back_unique(_buf);
  } else {
    _hist_list.push_back(_buf);
  }
  if (_hist_list.size() > static_cast<uint>(Config::instance()->getCmdDialogHistorySize())) {
    _hist_list.pop_front();
  }

  // Persist changes
  if (Config::instance()->getCmdDialogHistorySaveInterval() > 0
      && Config::instance()->getCmdDialogHistoryFile().size() > 0
      && ++_exec_count > Config::instance()->getCmdDialogHistorySaveInterval()) {
    _hist_list.save(Config::instance()->getCmdDialogHistoryFile());
    _exec_count = 0;
  }

    
    // Check if it's a valid Action, if not we assume it's a command and try
    // to execute it.
    string buf_mb(Util::to_mb_str(_buf));
    if (! Config::instance()->parseAction(buf_mb, _ae.action_list.back(), KEYGRABBER_OK)) {
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
        InputDialog::unmapWindow();
        _wo_ref = 0;
        bufClear();
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
        _buf_on_complete = _buf;
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
CmdDialog::mapCentered(const std::string &buf, bool focus, PWinObj *wo_ref)
{
    _wo_ref = wo_ref ? wo_ref : _wo_ref;

    InputDialog::mapCentered(buf, focus, wo_ref);
}
