//
// KeyboardMoveResizeEventHandler.hh for pekwm
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
#include "KeyGrabber.hh"
#include "StatusWindow.hh"

class KeyboardMoveResizeEventHandler : public EventHandler,
                                       public Observer
{
public:
    KeyboardMoveResizeEventHandler(Config* cfg, KeyGrabber *key_grabber,
                                   PDecor* decor)
        : _cfg(cfg),
          _key_grabber(key_grabber),
          _outline(! cfg->getOpaqueMove() || ! cfg->getOpaqueResize()),
          _show_status_window(cfg->isShowStatusWindow()),
          _center_on_root(cfg->isShowStatusWindowOnRoot()),
          _init(false),
          _decor(decor)
    {
        decor->getGeometry(_gm);
        decor->getGeometry(_old_gm);
        decor->addObserver(this);
    }
    virtual ~KeyboardMoveResizeEventHandler(void)
    {
        if (_decor) {
            _decor->removeObserver(this);
        }
        stopMoveResize();
    }

    virtual void notify(Observable *observable,
                        Observation *observation) override
    {
        if (observable == _decor) {
            TRACE("decor " << _decor << " lost while keyboard move/resize");
            _decor = nullptr;
        }
    }

    virtual bool
    initEventHandler(void) override
    {
        if (! X11::grabPointer(X11::getRoot(), NoEventMask, CURSOR_MOVE)) {
            return false;
        }
        if (! X11::grabKeyboard(X11::getRoot())) {
            X11::ungrabPointer();
            return false;
        }

        if (_outline) {
            X11::grabServer();
            drawOutline(); // draw for initial clear
        }
        updateStatusWindow(true);

        _init = true;
        return true;
    }

    virtual EventHandler::Result
    handleButtonPressEvent(XButtonEvent *ev) override
    {
        // mark as processed disabling wm processing of these events.
        return EventHandler::EVENT_PROCESSED;
    }

    virtual EventHandler::Result
    handleButtonReleaseEvent(XButtonEvent *ev) override
    {
        // mark as processed disabling wm processing of these events.
        return EventHandler::EVENT_PROCESSED;
    }

    virtual EventHandler::Result
    handleMotionNotifyEvent(XMotionEvent *ev) override
    {
        // mark as processed disabling wm processing of these events.
        return EventHandler::EVENT_PROCESSED;
    }

    virtual EventHandler::Result
    handleKeyEvent(XKeyEvent *ev) override
    {
        if (! _decor) {
            return stopMoveResize();
        } else if (ev->type == KeyRelease) {
            return EventHandler::EVENT_PROCESSED;
        }

        auto res = EventHandler::EVENT_PROCESSED;
        auto ae = _key_grabber->findMoveResizeAction(ev);
        if (ae == nullptr) {
            return res;
        }

        drawOutline(); // clear previous draw

        for (auto it : ae->action_list) {
            res = runMoveResizeAction(it);
            if (res == EventHandler::EVENT_STOP_PROCESSED) {
                break;
            }
        }

        drawOutline();
        updateStatusWindow(false);

        if (res == EventHandler::EVENT_STOP_PROCESSED
            || res == EventHandler::EVENT_STOP_SKIP) {
            stopMoveResize();
        }

        return res;
    }

    EventHandler::Result
    runMoveResizeAction(const Action& action)
    {
        switch (action.getAction()) {
        case MOVE_HORIZONTAL:
            if (action.getParamI(1) == UNIT_PERCENT) {
                Geometry head;
                X11::getHeadInfo(X11Util::getNearestHead(*_decor), head);
                _gm.x += (action.getParamI(0)
                          * static_cast<int>(head.width)) / 100;
            } else {
                _gm.x += action.getParamI(0);
            }
            if (! _outline) {
                _decor->move(_gm.x, _gm.y);
            }
            break;
        case MOVE_VERTICAL:
            if (action.getParamI(1) == UNIT_PERCENT) {
                Geometry head;
                X11::getHeadInfo(X11Util::getNearestHead(*_decor), head);
                _gm.y += (action.getParamI(0)
                          * static_cast<int>(head.height)) / 100;
            } else {
                _gm.y +=  action.getParamI(0);
            }
            if (! _outline) {
                _decor->move(_gm.x, _gm.y);
            }
            break;
        case RESIZE_HORIZONTAL:
            if (action.getParamI(1) == UNIT_PERCENT) {
                Geometry head;
                X11::getHeadInfo(X11Util::getNearestHead(*_decor), head);
                _gm.width += (action.getParamI(0)
                              * static_cast<int>(head.width)) / 100;
            } else {
                _gm.width +=  action.getParamI(0);
            }
            if (! _outline) {
                _decor->resize(_gm.width, _gm.height);
            }
            break;
        case RESIZE_VERTICAL:
            if (action.getParamI(1) == UNIT_PERCENT) {
                Geometry head;
                X11::getHeadInfo(X11Util::getNearestHead(*_decor), head);
                _gm.height += (action.getParamI(0)
                               * static_cast<int>(head.height)) / 100;
            } else {
                _gm.height +=  action.getParamI(0);
            }
            if (! _outline) {
                _decor->resize(_gm.width, _gm.height);
            }
            break;
        case MOVE_SNAP:
            PDecor::checkSnap(_decor, _gm);
            if (! _outline) {
                _decor->move(_gm.x, _gm.y);
            }
            break;
        case MOVE_CANCEL:
            _gm = _old_gm; // restore position

            if (! _outline) {
                _decor->move(_gm.x, _gm.y);
                _decor->resize(_gm.width, _gm.height);
            }

            return EventHandler::EVENT_STOP_PROCESSED;
        case MOVE_END:
            if (_outline) {
                if (_gm.x != _old_gm.x
                    || _gm.y != _old_gm.y) {
                    _decor->move(_gm.x, _gm.y);
                }
                if (_gm.width != _old_gm.width
                    || _gm.height != _old_gm.height) {
                    _decor->resize(_gm.width, _gm.height);
                }
            }
            return EventHandler::EVENT_STOP_PROCESSED;
        default:
            break;
        }

        return EventHandler::EVENT_PROCESSED;
    }

    void drawOutline(void) {
        if (_outline) {
            _decor->drawOutline(_gm);
        }
    }

    void updateStatusWindow(bool map) {
        if (_show_status_window) {
            wchar_t buf[128];
            _decor->getDecorInfo(buf, 128, _gm);

            auto sw = pekwm::statusWindow();
            if (map) {
                // draw before map to avoid resize right after the
                // window is mapped.
                sw->draw(buf, true, _center_on_root ? 0 : &_gm);
                sw->mapWindowRaised();
            }
            sw->draw(buf, true, _center_on_root ? 0 : &_gm);
        }
    }

    EventHandler::Result stopMoveResize(void)
    {
        if (_init) {
            if (_show_status_window) {
                pekwm::statusWindow()->unmapWindow();
            }
            if (_outline) {
                drawOutline();
                X11::ungrabServer(true);
            }
            X11::ungrabPointer();
            X11::ungrabKeyboard();
            _init = false;
        }
        return EventHandler::EVENT_STOP_SKIP;
    }

private:
    Config *_cfg;
    KeyGrabber *_key_grabber;
    bool _outline;
    bool _show_status_window;
    bool _center_on_root;

    Geometry _gm;
    Geometry _old_gm;

    bool _init;
    PDecor *_decor;
};
