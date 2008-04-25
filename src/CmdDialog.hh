//
// CmdDialog.hh for pekwm
// Copyright © 2004-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CMD_DIALOG_HH_
#define _CMD_DIALOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Action.hh"
#include "PDecor.hh"
#include "Theme.hh"

#include <list>
#include <string>

//! @brief CmdDialog presenting the user with an pekwm action command line.
class CmdDialog : public PDecor
{
public:
    CmdDialog(Display *dpy, Theme *theme, const std::wstring &title);
    virtual ~CmdDialog(void);

    // BEGIN - PWinObj interface
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    // END - PWinObj interface

    // wo event interface
    ActionEvent *handleButtonPress(XButtonEvent *ev);
    ActionEvent *handleKeyPress(XKeyEvent *ev);
    ActionEvent *handleExposeEvent(XExposeEvent *ev);

    void setTitle(const std::wstring &title);

    //! @brief Returns the PWinObj the CmdDialog executes actions on.
    inline PWinObj *getWORef(void) { return _wo_ref; }
    //! @brief Sets the PWinObj the CmdDialog executes actions on.
    inline void setWORef(PWinObj *wo) { _wo_ref = wo; }

    void mapCentered(const std::string &buf, bool focus,
                     PWinObj *wo_ref = NULL);
    void moveCentered(PWinObj *wo);

private:
    // BEGIN - PDecor interface
    virtual void loadTheme(void);
    // END - PDecor interface
    void unloadTheme(void);

    void render(void);

    ActionEvent *close(void);
    ActionEvent *exec(void);
    void complete(void);

    void bufAdd(XKeyEvent *ev);
    void bufRemove(void);
    void bufClear(void);
    void bufKill(void);
    void bufChangePos(int off);

    void bufChanged(void); // recalculates buf position

    void histNext(void);
    void histPrev(void);

private:
    Theme::TextDialogData *_cmd_data;

    PWinObj *_cmd_wo;
    PDecor::TitleItem _title;

    Pixmap _bg;

    // action event, for event handling
    ActionEvent _ae;
    PWinObj *_wo_ref;

    // content related
    std::wstring _buf;
    uint _pos, _buf_off, _buf_chars; // position, start and num display

    // history
    std::wstring _hist_new; // the one we started editing on
    std::list<std::wstring> _hist_list;
    std::list<std::wstring>::iterator _hist_it;
};

#endif // _CMD_DIALOG_HH_
