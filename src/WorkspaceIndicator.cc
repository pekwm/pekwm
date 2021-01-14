//
// WorkspaceIndicator.cc for pekwm
// Copyright (C) 2009-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "Config.hh"
#include "Workspaces.hh"
#include "WorkspaceIndicator.hh"
#include "x11.hh"

/**
 * Display constructor
 */
WorkspaceIndicator::Display::Display(PWinObj *parent)
  : PWinObj(false),
    _pixmap(None)
{
    _parent = parent;
    // Do not give the indicator focus, it doesn't handle input
    _focusable = false;

    XSetWindowAttributes attr;
    attr.override_redirect = false;
    attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
                    FocusChangeMask|KeyPressMask|KeyReleaseMask;
    _window = XCreateWindow(X11::getDpy(), _parent->getWindow(), 0, 0, 1, 1, 0,
                            CopyFromParent, InputOutput, CopyFromParent,
                            CWOverrideRedirect|CWEventMask, &attr);
}

/**
 * Display destructor
 */
WorkspaceIndicator::Display::~Display(void)
{
    XDestroyWindow(X11::getDpy(), _window);
    X11::freePixmap(_pixmap);
}

/**
 * Get required size to render workspaces.
 */
bool
WorkspaceIndicator::Display::getSizeRequest(Geometry &gm)
{
    Geometry head;
    uint head_nr = X11::getNearestHead(_parent->getX(), _parent->getY());
    X11::getHeadInfo(head_nr, head);

    uint head_size = std::min(head.width, head.height)
        / Config::instance()->getWorkspaceIndicatorScale();
    gm.x = gm.y = 0;
    gm.width = head_size * Workspaces::getPerRow() + getPaddingHorizontal();
    gm.height = head_size * Workspaces::getRows() + getPaddingVertical();

    return true;
}

/**
 * Render workspace view to pixmap
 */
void
WorkspaceIndicator::Display::render(void)
{
    auto data = Theme::instance()->getWorkspaceIndicatorData();

    // Make sure pixmap has correct size
    X11::freePixmap(_pixmap);
    _pixmap = X11::createPixmap(_gm.width, _gm.height);

    // Render background
    data.texture_background->render(_pixmap, 0, 0, _gm.width, _gm.height);

    // Render workspace grid, then active workspace fill and end with
    // rendering active workspace number and name
    renderWorkspaces(data.edge_padding, data.edge_padding,
                     _gm.width - getPaddingHorizontal(),
                     _gm.height - getPaddingVertical());

    data.font->setColor(data.font_color);
    data.font->draw(_pixmap,
                    data.edge_padding,
                    _gm.height - data.edge_padding - data.font->getHeight(),
                    Workspaces::getActWorkspace().getName(),
                    0 /* max_chars */,
                    _gm.width - data.edge_padding * 2 /* max_width */);

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
WorkspaceIndicator::Display::renderWorkspaces(int x, int y,
                                              uint width, uint height)
{
    auto data = Theme::instance()->getWorkspaceIndicatorData();

    uint per_row = Workspaces::getPerRow();
    uint rows = Workspaces::getRows();

    uint ws_width = width / per_row;
    uint ws_height = height / rows;

    // Starting positions of the workspace squares
    uint x_pos = x;
    uint y_pos = y;

    std::vector<Workspace>::size_type i=0;
    for (uint row = 0; i < Workspaces::size(); ++i) {
        // Check for next row
        if (Workspaces::getRow(i) > row) {
            row = Workspaces::getRow(i);

            x_pos = x;
            y_pos += ws_height + data.workspace_padding;
        }

        auto tex = i == Workspaces::getActive()
            ? data.texture_workspace_act : data.texture_workspace;
        tex->render(_pixmap, x_pos, y_pos, ws_width, ws_height);

        x_pos += ws_width + data.workspace_padding;
    }
}

/**
 * Get horizontal padding for window around workspaces.
 */
uint
WorkspaceIndicator::Display::getPaddingHorizontal(void)
{
    auto data = Theme::instance()->getWorkspaceIndicatorData();
    return (data.edge_padding * 2 + data.workspace_padding
            * (Workspaces::getPerRow() - 1));
}

/**
 * Get vertical padding for window around workspaces.
 */
uint
WorkspaceIndicator::Display::getPaddingVertical(void)
{
    auto data = Theme::instance()->getWorkspaceIndicatorData();
    return (data.edge_padding * 3 + data.font->getHeight()
            + data.workspace_padding * (Workspaces::getRows() - 1));
}

/**
 * WorkspaceIndicator constructor
 */
WorkspaceIndicator::WorkspaceIndicator()
    : PDecor("WORKSPACEINDICATOR"),
      _display_wo(this)
{
    _type = PWinObj::WO_WORKSPACE_INDICATOR;
    setLayer(LAYER_NONE); // Make sure this goes on top of everything
    _hidden = true; // Do not include in workspace handling etc

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
    Workspaces::insert(this);
    woListAdd(this);
    _wo_map[_window] = this;

    setOpacity(Config::instance()->getWorkspaceIndicatorOpacity());
}

/**
 * WorkspaceIndicator destructor
 */
WorkspaceIndicator::~WorkspaceIndicator(void)
{
    removeChild(&_display_wo, false);

    // Un-register ourselves
    Workspaces::remove(this);
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
    X11::getHeadInfo(X11::getCurrHead(), head);

    _display_wo.getSizeRequest(request);
    resizeChild(request.width, request.height);

    move(head.x + (head.width - _gm.width) / 2,
         head.y + (head.height - _gm.height) / 2);

    // Render workspaces
    _display_wo.render();
}
