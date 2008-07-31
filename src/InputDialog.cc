//
// CmdDialog.hh for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <list>

#include "InputDialog.hh"
#include "KeyGrabber.hh"
#include "PScreen.hh"
#include "PixmapHandler.hh"
#include "ScreenResources.hh"
#include "Workspaces.hh"

extern "C" {
#include <X11/Xutil.h>
}

using std::list;
using std::wstring;

/**
 * InputDialog constructor.
 */
InputDialog::InputDialog(Display *dpy, Theme *theme, const std::wstring &title)
  : PDecor(dpy, theme, "INPUTDIALOG"),
    _data(theme->getCmdDialogData()),
    _pixmap_bg(None), _pos(0), _buf_off(0), _buf_chars(0)
{
    // PWinObj attributes
    _layer = LAYER_NONE; // hack, goes over LAYER_MENU
    _hidden = true; // don't care about it when changing worskpace etc

    // Add action to list, going to be used from close and exec
    ::Action action;
    _ae.action_list.push_back(action);

    titleAdd(&_title);
    titleSetActive(0);
    setTitle(title);

    _text_wo = new PWinObj(_dpy);
    XSetWindowAttributes attr;
    attr.override_redirect = false;
    attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
    FocusChangeMask|KeyPressMask|KeyReleaseMask;
    _text_wo->setWindow(XCreateWindow(_dpy, _window,
                                   0, 0, 1, 1, 0,
                                   CopyFromParent, InputOutput, CopyFromParent,
                                   CWOverrideRedirect|CWEventMask, &attr));

    addChild(_text_wo);
    addChildWindow(_text_wo->getWindow());
    activateChild(_text_wo);
    _text_wo->mapWindow();

    // setup texture, size etc
    loadTheme();

    Workspaces::instance()->insert(this);
    woListAdd(this);
    _wo_map[_window] = this;
}

/**
 * InputDialog destructor.
 */
InputDialog::~InputDialog(void)
{
    Workspaces::instance()->remove(this);
    _wo_map.erase(_window);
    woListRemove(this);

    // Free resources
    if (_text_wo) {
        _child_list.remove(_text_wo);
        removeChildWindow(_text_wo->getWindow());
        XDestroyWindow(_dpy, _text_wo->getWindow());
        delete _text_wo;
    }

    unloadTheme();
}

/**
 * Handles ButtonPress, moving the text cursor
 */
ActionEvent*
InputDialog::handleButtonPress(XButtonEvent *ev)
{
    if (*_text_wo == ev->window) {
        // FIXME: move cursor
        return NULL;
    } else {
        return PDecor::handleButtonPress(ev);
    }
}

/**
 * Handles KeyPress, editing the buffer
 */
ActionEvent*
InputDialog::handleKeyPress(XKeyEvent *ev)
{
    ActionEvent *c_ae, *ae = NULL;

    if (! (c_ae = KeyGrabber::instance()->findAction(ev, _type))) {
        list<Action>::iterator it(c_ae->action_list.begin());
        for (; it != c_ae->action_list.end(); ++it) {
            switch (it->getAction()) {
            case INPUT_INSERT:
                bufAdd(ev);
                break;
            case INPUT_REMOVE:
                bufRemove();
                break;
            case INPUT_CLEAR:
                bufClear();
                break;
            case INPUT_CLEARFROMCURSOR:
                bufKill();
                break;
            case INPUT_EXEC:
                ae = exec();
                break;
            case INPUT_CLOSE:
                ae = close();
                break;
            case INPUT_COMPLETE:
                break;
            case INPUT_CURS_NEXT:
                bufChangePos(1);
                break;
            case INPUT_CURS_PREV:
                bufChangePos(-1);
                break;
            case INPUT_CURS_BEGIN:
                _pos = 0;
                break;
            case INPUT_CURS_END:
                _pos = _buf.size();
                break;
            case INPUT_HIST_NEXT:
                histNext();
                break;
            case INPUT_HIST_PREV:
                histPrev();
                break;
            case INPUT_NO_ACTION:
            default:
                // do nothing, shouldn't happen
                break;
            };
        }

        // something ( most likely ) changed, redraw the window
        if (! ae) {
            bufChanged();
            render();
        }
    }

    return ae;
}

/**
 * Handles ExposeEvent, redraw when ev->count == 0
 */
ActionEvent*
InputDialog::handleExposeEvent(XExposeEvent *ev)
{
    if (ev->count > 0) {
        return NULL;
    }
    render();
    return NULL;
}

/**
 * Maps the InputDialog center on the PWinObj it executes actions on.
 *
 * @param buf Buffer content.
 * @param focus Give input focus if true.
 * @param wo_ref PWinObj reference, defaults to NULL which does not update.
 */
void
InputDialog::mapCentered(const std::string &buf, bool focus, PWinObj *wo_ref)
{
    // Setup data
    _hist_it = _hist_list.end();

    _buf = Util::to_wide_str(buf);
    _pos = _buf.size();
    bufChanged();

    // Update position
    moveCentered(wo_ref);

    // Map and render
    PDecor::mapWindowRaised();
    render();

    // Give input focus if requested
    if (focus) {
        giveInputFocus();
    }
}

/**
 * Moves to center of wo.
 *
 * @param wo PWinObj to center on.
 */
void
InputDialog::moveCentered(PWinObj *wo)
{
    // Fallback wo on root.
    if (! wo) {
        wo = PWinObj::getRootPWinObj();
    }

    // Make sure position is inside head.
    Geometry head;
    uint head_nr = PScreen::instance()->getNearestHead(wo->getX() + (wo->getWidth() / 2),
                                                     wo->getY() + (wo->getHeight() / 2));
    PScreen::instance()->getHeadInfo(head_nr, head);

    // Make sure X is inside head.
    int new_x = wo->getX() + (static_cast<int>(wo->getWidth()) - static_cast<int>(_gm.width)) / 2;
    if (new_x < head.x) {
        new_x = head.x;
    } else if ((new_x + _gm.width) > (head.x + head.width)) {
        new_x = head.x + head.width - _gm.width;
    }

    // Make sure Y is inside head.
    int new_y = wo->getY() + (static_cast<int>(wo->getHeight()) - static_cast<int>(_gm.height)) / 2;
    if (new_y < head.y) {
        new_y = head.y;
    } else if ((new_y + _gm.height) > (head.y + head.height)) {
        new_y = head.y + head.height - _gm.height;
    }

    // Update position.
    move(new_x, new_y);    
}

/**
 * Sets title of decor
 */
void
InputDialog::setTitle(const std::wstring &title)
{
    _title.setReal(title);
}

/**
 * Maps window, overloaded to refresh content of window after mapping.
 */
void
InputDialog::mapWindow(void)
{
    if (! _mapped) {
        // Correct size for current head before mapping
        updateSize();
    
        PDecor::mapWindow();
        render();
    }
}

/**
 * Sets background and size
 */
void
InputDialog::loadTheme(void)
{
    _data = _theme->getCmdDialogData();
    updateSize();
}

/**
 * Frees resources
 */
void
InputDialog::unloadTheme(void)
{
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_pixmap_bg);
}

/**
 * Renders _buf onto _text_wo
 */
void
InputDialog::render(void)
{
    _text_wo->clear();

    // draw buf content
    _data->getFont()->setColor(_data->getColor());

    _data->getFont()->draw(_text_wo->getWindow(), _data->getPad(PAD_LEFT), _data->getPad(PAD_UP),
                          _buf.c_str() + _buf_off, _buf_chars);

    // draw cursor
    uint pos = _data->getPad(PAD_LEFT);
    if (_pos > 0) {
        pos = _data->getFont()->getWidth(_buf.c_str() + _buf_off,  _pos - _buf_off) + 1;
    }

    _data->getFont()->draw(_text_wo->getWindow(), pos, _data->getPad(PAD_UP), L"|");
}

/**
 * Generates ACTION_CLOSE closing dialog.
 *
 * @return Pointer to ActionEvent.
 */
ActionEvent*
InputDialog::close(void)
{
    _ae.action_list.back().setAction(ACTION_NO);
    return &_ae;
}

/**
 * Adds char to buffer
 */
void
InputDialog::bufAdd(XKeyEvent *ev)
{
    char c_return[64];
    memset(c_return, '\0', 64);

    XLookupString(ev, c_return, 64, NULL, NULL);

    // Add wide string to buffer counting position
    wstring buf_ret(Util::to_wide_str(c_return));
    for (unsigned int i = 0; i < buf_ret.size(); ++i) {
        if (iswprint(buf_ret[i])) {
            _buf.insert(_buf.begin() + _pos++, buf_ret[i]);
        }
    }
}

/**
 * Removes char from buffer
 */
void
InputDialog::bufRemove(void)
{
    if ((_pos > _buf.size()) || (_pos == 0) || (_buf.size() == 0)) {
        return;
    }

    _buf.erase(_buf.begin() + --_pos);
}

/**
 * Clears the buffer, resets status
 */
void
InputDialog::bufClear(void)
{
    _buf = L""; // old gcc doesn't know about .clear()
    _pos = _buf_off = _buf_chars = 0;
}

/**
 * Removes buffer content after cursor position
 */
void
InputDialog::bufKill(void)
{
    _buf.resize(_pos);
}

/**
 * Moves the marker
 */
void
InputDialog::bufChangePos(int off)
{
    if ((signed(_pos) + off) < 0) {
        _pos = 0;
    } else if (unsigned(_pos + off) > _buf.size()) {
        _pos = _buf.size();
    } else {
        _pos += off;
    }
}

/**
 * Recalculates, _buf_off and _buf_chars
 */
void
InputDialog::bufChanged(void)
{
    PFont *font =  _data->getFont(); // convenience

    // complete string doesn't fit in the window OR
    // we don't fit in the first set
    if ((_pos > 0)
      && (font->getWidth(_buf.c_str()) > _text_wo->getWidth())
      && (font->getWidth(_buf.c_str(), _pos) > _text_wo->getWidth())) {

        // increase position until it all fits
        for (_buf_off = 0; _buf_off < _pos; ++_buf_off) {
            if (font->getWidth(_buf.c_str() + _buf_off, _buf.size() - _buf_off)
                    < _text_wo->getWidth()) {
                break;
            }
        }

        _buf_chars = _buf.size() - _buf_off;
    } else {
        _buf_off = 0;
        _buf_chars = _buf.size();
    }
}

/**
 * Sets the buffer to the next item in the history.
 */
void
InputDialog::histNext(void)
{
    if (_hist_it == _hist_list.end()) {
        return; // nothing to do
    }

    // get next item, if at the end, restore the edit buffer
    ++_hist_it;
    if (_hist_it == _hist_list.end()) {
        _buf = _hist_new;
    } else {
        _buf = *_hist_it;
    }

    // move cursor to the end of line
    _pos = _buf.size();
}

/**
 * Sets the buffer to the previous item in the history.
 */
void
InputDialog::histPrev(void)
{
    if (_hist_it == _hist_list.begin()) {
        return; // nothing to do
    }

    // save item so we can restore the edit buffer later
    if (_hist_it == _hist_list.end()) {
        _hist_new = _buf;
    }

    // get prev item
    _buf = *(--_hist_it);

    // move cursor to the end of line
    _pos = _buf.size();
}

/**
 * Update command dialog size for view on current head.
 */
void
InputDialog::updateSize(void)
{
    Geometry head;
    PScreen::instance()->getHeadInfo(PScreen::instance()->getNearestHead(_gm.x, _gm.y), head);

    // Resize the child window and update the size depending.
    uint old_width = _gm.width;
    resizeChild(head.width / 3, _data->getFont()->getHeight() + _data->getPad(PAD_UP) + _data->getPad(PAD_DOWN));

    // If size was updated, replace the texture and recalculate display
    // buffer.
    if (old_width != _gm.width) {
        updatePixmapSize();
        bufChanged();
    }
}

/**
 * Update background pixmap size and redraw.
 */
void
InputDialog::updatePixmapSize(void)
{
    // Get new pixmap and render texture
    PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();
    pm->returnPixmap(_pixmap_bg);
    _pixmap_bg = pm->getPixmap(_text_wo->getWidth(), _text_wo->getHeight(), PScreen::instance()->getDepth());

    _data->getTexture()->render(_pixmap_bg, 0, 0, _text_wo->getWidth(), _text_wo->getHeight());
    _text_wo->setBackgroundPixmap(_pixmap_bg);
    _text_wo->clear();
}
