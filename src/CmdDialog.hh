//
// CmdDialog.hh for pekwm
// Copyright (C) 2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _CMD_DIALOG_HH_
#define _CMD_DIALOG_HH_

#include "Action.hh"

#include <list>
#include <string>

class PDecor;
class PDecor::TitleItem;
class Theme::TextDialogData;

//! @brief CmdDialog presenting the user with an pekwm action command line.
class CmdDialog : public PDecor
{
public:
    CmdDialog(Display *dpy, Theme *theme, const std::string &title);
    virtual ~CmdDialog(void);

    // wo event interface
    ActionEvent *handleButtonPress(XButtonEvent *ev);
    ActionEvent *handleKeyPress(XKeyEvent *ev);
    ActionEvent *handleExposeEvent(XExposeEvent *ev);

    void setTitle(const std::string &title);

    //! @brief Returns the PWinObj the CmdDialog executes actions on.
    inline PWinObj *getWORef(void) { return _wo_ref; }
    //! @brief Sets the PWinObj the CmdDialog executes actions on.
    inline void setWORef(PWinObj *wo) { _wo_ref = wo; }

    void mapCenteredOnWORef(void);

private:
    // BEGIN - PDecor interface
    virtual void loadTheme(void);
    // END - PDecor interface
    void unloadTheme(void);

    void render(void);

    ActionEvent *exec(void);
    void complete(void);

    void bufAdd(XKeyEvent *ev);
    void bufRemove(void);
    void bufClear(void);
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
    std::string _buf;
    uint _pos, _buf_off, _buf_chars; // position, start and num display

    // history
    std::string _hist_new; // the one we started editing on
    std::list<std::string> _hist_list;
    std::list<std::string>::iterator _hist_it;
};

#endif // _CMD_DIALOG_HH_
