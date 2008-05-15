//
// WorkspaceIndicator.hh for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WORKSPACE_INDICATOR_HH_
#define _WORKSPACE_INDICATOR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include "PWinObj.hh"
#include "PDecor.hh"
#include "Theme.hh"
#include "Timer.hh"

/**
 * Workspace indicator rendering a simple window with workspace layout
 * showing the number and name of the active workspace.
 */
class WorkspaceIndicator : public PDecor
{
public:
  /**
   * Display class rendering workspace layout in WorkspaceIndicator.
   */
  class Display : public PWinObj {
  public:
    Display(::Display *dpy, PWinObj *parent, Theme *theme);
    virtual ~Display(void);

    void render(void);

  private:
    void renderWorkspaces(int x, int y, uint width, uint height);

  private:
    Theme *_theme;
    Pixmap _pixmap; //!< Pixmap holding rendered workspace view
  };

  WorkspaceIndicator(::Display *dpy, Theme *theme, Timer<ActionPerformed> &timer);
  virtual ~WorkspaceIndicator(void);

  void render(void);
  void updateHideTimer(uint timeout);

private:
  Timer<ActionPerformed> &_timer; //!< Timer used to add unmap events to

  Display _display_wo; //!< Display winobj handling rendering of workspace status
  PDecor::TitleItem _title; //!< Title item added to the decor
  Timer<ActionPerformed>::timed_event_list_entry _timer_hide; //!< Timeout for hiding workspace indicator
  ActionEvent _action_hide; //!< ActionEvent for hiding the workspace indicator used in timer
};

#endif // _WORKSPACE_INDICATOR_HH_
