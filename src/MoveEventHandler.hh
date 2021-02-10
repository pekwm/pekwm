//
// MoveEvenHandler.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "Action.hh"
#include "Config.hh"
#include "EventHandler.hh"
#include "Observer.hh"
#include "StatusWindow.hh"

class MoveEventHandler : public EventHandler,
                         public Observer
{
public:
    MoveEventHandler(Config* cfg, PDecor* decor, int x_root, int y_root)
        : _cfg(cfg),
          _outline(!cfg->getOpaqueMove()),
          _show_status_window(cfg->isShowStatusWindow()),
          _center_on_root(cfg->isShowStatusWindowOnRoot()),
          _curr_edge(SCREEN_EDGE_NO),
          _decor(decor)
    {
        decor->getGeometry(_gm);
        decor->getGeometry(_last_gm);
        _x = x_root - _gm.x;
        _y = y_root - _gm.y;

        decor->addObserver(this);
    }
    virtual ~MoveEventHandler(void)
    {
        if (_decor) {
            _decor->removeObserver(this);
        }
    }

    virtual void notify(Observable *observable,
                        Observation *observation) override
    {
        if (observable == _decor) {
            TRACE("decor " << _decor << " lost while moving");
            _decor = nullptr;
        }
    }

    virtual EventHandler::Result
    handleButtonPressEvent(XButtonEvent *ev) override
    {
        if (! _decor
            || ! _decor->allowMove()
            || ! X11::grabPointer(X11::getRoot(),
                                  ButtonMotionMask|ButtonReleaseMask,
                                  CURSOR_MOVE)) {
            return EventHandler::EVENT_STOP_PROCESSED;
        }

        // Grab server to avoid any events causing garbage on the
        // screen as the outline is drawed as an inverted rectangle.
        if (_outline) {
            X11::grabServer();
            drawOutline();
        }

        if (_show_status_window) {
            wchar_t buf[128];
            _decor->getDecorInfo(buf, 128);

            auto sw = pekwm::statusWindow();
            sw->draw(buf, true, _center_on_root ? 0 : &_gm);
            sw->mapWindowRaised();
            sw->draw(buf, true, _center_on_root ? 0 : &_gm);
        }

        return EventHandler::EVENT_PROCESSED;
    }

    virtual EventHandler::Result
    handleButtonReleaseEvent(XButtonEvent *ev) override
    {
        stopMove();

        if (_decor) {
            _decor->move(_gm.x, _gm.y);
        }

        return EventHandler::EVENT_STOP_PROCESSED;
    }

    virtual EventHandler::Result
    handleMotionNotifyEvent(XMotionEvent *ev) override
    {
        if (! _decor || _decor->isFullscreen()) {
            return stopMove();
        }

        drawOutline(); //clear

        // Flush all pointer motion, no need to redraw and redraw.
        X11::removeMotionEvents();

        _gm.x = ev->x_root - _x;
        _gm.y = ev->y_root - _y;
        PDecor::checkSnap(_decor, _gm);

        if (! _outline && _gm != _last_gm) {
            _last_gm = _gm;
            X11::moveWindow(_decor->getWindow(), _gm.x, _gm.y);
        }

        auto edge = doMoveEdgeFind(ev->x_root, ev->y_root);
        if (edge != _curr_edge) {
            _curr_edge = edge;
            if (edge != SCREEN_EDGE_NO) {
                doMoveEdgeAction(ev, edge);
            }
        }

        wchar_t buf[128];
        _decor->getDecorInfo(buf, 128);
        if (_show_status_window) {
            pekwm::statusWindow()->draw(buf, true, _center_on_root ? 0 : &_gm);
        }

        drawOutline();

        return EventHandler::EVENT_PROCESSED;
    }

private:
    EventHandler::Result stopMove(void) {
        if (_show_status_window) {
            pekwm::statusWindow()->unmapWindow();
        }
        if (_outline) {
            drawOutline(); // clear
            X11::ungrabServer(true);
        }
        X11::ungrabPointer();
        return EventHandler::EVENT_STOP_SKIP;
    }

    void drawOutline(void) {
        if (_outline) {
            _decor->drawOutline(_gm);
        }
    }

    EdgeType doMoveEdgeFind(int x, int y)
    {
        auto edge = SCREEN_EDGE_NO;
        if (x <= signed(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_LEFT))) {
            edge = SCREEN_EDGE_LEFT;
        } else if (x >= signed(X11::getWidth() -
                               _cfg->getScreenEdgeSize(SCREEN_EDGE_RIGHT))) {
            edge = SCREEN_EDGE_RIGHT;
        } else if (y <= signed(_cfg->getScreenEdgeSize(SCREEN_EDGE_TOP))) {
            edge = SCREEN_EDGE_TOP;
        } else if (y >= signed(X11::getHeight() -
                               _cfg->getScreenEdgeSize(SCREEN_EDGE_BOTTOM))) {
            edge = SCREEN_EDGE_BOTTOM;
        }
        return edge;
    }

    void doMoveEdgeAction(XMotionEvent *ev, EdgeType edge)
    {
        uint button = X11::getButtonFromState(ev->state);
        auto ae =
            ActionHandler::findMouseAction(button, ev->state,
                                           MOUSE_EVENT_ENTER_MOVING,
                                           _cfg->getEdgeListFromPosition(edge));
        if (ae) {
            ActionPerformed ap(_decor, *ae);
            ap.type = ev->type;
            ap.event.motion = ev;

            pekwm::actionHandler()->handleAction(ap);
        }
    }


private:
    Config *_cfg;

    bool _outline;
    bool _show_status_window;
    bool _center_on_root;
    Geometry _gm;
    Geometry _last_gm;
    EdgeType _curr_edge;

    int _x;
    int _y;

    PDecor *_decor;
};
