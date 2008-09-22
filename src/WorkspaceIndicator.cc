//
// WorkspaceIndicator.hh for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#include "Config.hh"
#include "PixmapHandler.hh"
#include "PScreen.hh"
#include "ScreenResources.hh"
#include "Workspaces.hh"
#include "WorkspaceIndicator.hh"

using std::cerr;
using std::endl;
using std::vector;

/**
 * Display constructor
 */
WorkspaceIndicator::Display::Display(::Display *dpy, PWinObj *parent, Theme *theme)
  : PWinObj(dpy),
    _theme(theme), _pixmap(None)
{
    _parent = parent;
    // Do not give the indicator focus, it doesn't handle input
    _focusable = false;

    XSetWindowAttributes attr;
    attr.override_redirect = false;
    attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
                    FocusChangeMask|KeyPressMask|KeyReleaseMask;
    _window = XCreateWindow(_dpy, _parent->getWindow(), 0, 0, 1, 1, 0,
                            CopyFromParent, InputOutput, CopyFromParent,
                            CWOverrideRedirect|CWEventMask, &attr);
}

/**
 * Display destructor
 */
WorkspaceIndicator::Display::~Display(void)
{
    XDestroyWindow(_dpy, _window);
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_pixmap);
}

/**
 * Get required size to render workspaces.
 */
bool
WorkspaceIndicator::Display::getSizeRequest(Geometry &gm)
{
    Geometry head;
    uint head_nr = PScreen::instance()->getNearestHead(_parent->getX(), _parent->getY());
    PScreen::instance()->getHeadInfo(head_nr, head);

    uint head_size = std::min(head.width, head.height) / Config::instance()->getWorkspaceIndicatorScale();
    gm.x = gm.y = 0;
    gm.width = head_size * Workspaces::instance()->getPerRow() + getPaddingHorizontal();
    gm.height = head_size * Workspaces::instance()->getRows() + getPaddingVertical();

    return true;
}

/**
 * Render workspace view to pixmap
 */
void
WorkspaceIndicator::Display::render(void)
{
    Theme::WorkspaceIndicatorData &data(_theme->getWorkspaceIndicatorData());

    // Make sure pixmap has correct size
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_pixmap);
    _pixmap = ScreenResources::instance()->getPixmapHandler()->getPixmap(_gm.width, _gm.height,
                                                                        PScreen::instance()->getDepth());

    // Render background
    data.texture_background->render(_pixmap, 0, 0, _gm.width, _gm.height);

    // Render workspace grid, then active workspace fill and end with
    // rendering active workspace number and name
    renderWorkspaces(data.edge_padding, data.edge_padding,
                     _gm.width - getPaddingHorizontal(),
                     _gm.height - getPaddingVertical());

    data.font->setColor(data.font_color);
    data.font->draw(_pixmap, data.edge_padding, _gm.height - data.edge_padding - data.font->getHeight(),
                     Workspaces::instance()->getActiveWorkspace()->getName(),
                     0 /* max_chars */, _gm.width - data.edge_padding * 2 /* max_width */);

    // Refresh
    setBackgroundPixmap(_pixmap);
    clear();
}

/**
 * Render workspace part of indication
 *
 * @param x X offset on drawable
 * @param y Y offset on drawable
 * @param width Allowed width to use
 * @param height Allowed height to use
 */
void
WorkspaceIndicator::Display::renderWorkspaces(int x, int y, uint width, uint height)
{
    Theme::WorkspaceIndicatorData &data(_theme->getWorkspaceIndicatorData());

    uint per_row = Workspaces::instance()->getPerRow();
    uint rows = Workspaces::instance()->getRows();

    uint ws_width = width / per_row;
    uint ws_height = height / rows;

    // Starting positions of the workspace squares
    uint x_pos = x;
    uint y_pos = y;

    vector<Workspaces::Workspace*>::iterator it(Workspaces::instance()->ws_begin());

    for (uint row = 0; it != Workspaces::instance()->ws_end(); ++it) {
        // Check for next row
        if (Workspaces::instance()->getRow((*it)->getNumber()) > row) {
            row = Workspaces::instance()->getRow((*it)->getNumber());

            x_pos = x;
            y_pos += ws_height + data.workspace_padding;
        }

        if ((*it)->getNumber() == Workspaces::instance()->getActive()) {
            data.texture_workspace_act->render(_pixmap, x_pos, y_pos, ws_width, ws_height);
        } else {
            data.texture_workspace->render(_pixmap, x_pos, y_pos, ws_width, ws_height);
        }

        x_pos += ws_width + data.workspace_padding;
    }
}

/**
 * Get horizontal padding for window around workspaces.
 */
uint
WorkspaceIndicator::Display::getPaddingHorizontal(void)
{
    Theme::WorkspaceIndicatorData &data(_theme->getWorkspaceIndicatorData());

    return (data.edge_padding * 2 + data.workspace_padding * (Workspaces::instance()->getPerRow() - 1));
}

/**
 * Get vertical padding for window around workspaces.
 */
uint
WorkspaceIndicator::Display::getPaddingVertical(void)
{
    Theme::WorkspaceIndicatorData &data(_theme->getWorkspaceIndicatorData());

    return (data.edge_padding * 3 + data.font->getHeight()
            + data.workspace_padding * (Workspaces::instance()->getRows() - 1));
}

/**
 * WorkspaceIndicator constructor
 */
WorkspaceIndicator::WorkspaceIndicator(::Display *dpy, Theme *theme, Timer<ActionPerformed> &timer)
  : PDecor(dpy, theme, "WORKSPACEINDICATOR"),
    _timer(timer), _display_wo(dpy, this, theme),
    _timer_hide(0)
{
    _type = PWinObj::WO_WORKSPACE_INDICATOR;
    _layer = LAYER_NONE; // Make sure this goes on top of everything
    _hidden = true; // Do not include in workspace handling etc

    // Add hide action to timer action
    _action_hide.action_list.push_back(Action(ACTION_HIDE_WORKSPACE_INDICATOR));

    // Add title
    titleAdd(&_title);
    titleSetActive(0);
    _title.setReal(L"Workspace");

    // Add display window
    addChild(&_display_wo);
    activateChild(&_display_wo);
    _display_wo.mapWindow();

    // Load theme data, horay for pretty colors
    loadTheme();

    // Register ourselves
    Workspaces::instance()->insert(this);
    woListAdd(this);
    _wo_map[_window] = this;
}

/**
 * WorkspaceIndicator destructor
 */
WorkspaceIndicator::~WorkspaceIndicator(void)
{
    // Un-register ourselves
    Workspaces::instance()->remove(this);
    _wo_map.erase(_window);
    woListRemove(this);
}

/**
 * Resize indicator and render
 */
void
WorkspaceIndicator::render(void)
{
    // Center on head
    Geometry head, request;
    PScreen::instance()->getHeadInfo(PScreen::instance()->getCurrHead(), head);

    _display_wo.getSizeRequest(request);
    resizeChild(request.width, request.height);

    move(head.x + (head.width - _gm.width) / 2,
         head.y + (head.height - _gm.height) / 2);

    // Render workspaces
    _display_wo.render();
}

/**
 * Remove previous timer and add new hide timer for timeout seconds in
 * the future.
 */
void
WorkspaceIndicator::updateHideTimer(uint timeout)
{
    if (_timer_hide) {
        _timer.remove(_timer_hide);
    }
    _timer_hide = _timer.add(timeout, ActionPerformed(this, _action_hide));
}
