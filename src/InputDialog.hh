//
// InputDialog.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include <map>

extern "C" {
#include <X11/Xlib.h>
}

#include "PWinObj.hh"
#include "PWinObjReference.hh"
#include "PDecor.hh"

/**
 * Base for windows handling text input.
 */
class InputDialog : public PDecor,
                    public PWinObjReference {
public:
    InputDialog(const std::wstring &title);
    virtual ~InputDialog(void);

    // BEGIN - PWinObj interface
    virtual void mapWindow(void);

    // PWinObj event interface
    ActionEvent *handleButtonPress(XButtonEvent *ev);
    ActionEvent *handleKeyPress(XKeyEvent *ev);
    ActionEvent *handleExposeEvent(XExposeEvent *ev);
    // END - PWinObj interface

    void setTitle(const std::wstring &title);

    void loadTheme(void);
    void unloadTheme(void);
    void render(void);

    static void reloadKeysymMap(void);

    virtual void mapCentered(const std::string &buf, const Geometry &gm,
                             PWinObj *wo_ref=0);
    virtual void moveCentered(const Geometry &head, const Geometry &gm);

protected:
    virtual ActionEvent *close(void);
    virtual ActionEvent *exec(void) { return 0; }
    virtual void complete(void);
    virtual void completeAbort(void);
    virtual void completeReset(void);

    virtual void bufAdd(XKeyEvent *ev);
    virtual void bufRemove(void);
    virtual void bufClear(void);
    virtual void bufKill(void);
    virtual void bufChangePos(int off);

    virtual void bufChanged(void); // recalculates buf position

    virtual void histNext(void);
    virtual void histPrev(void);

    virtual void updateSize(const Geometry &head);
    virtual void updatePixmapSize(void);

    void getInputSize(const Geometry &head, uint &width, uint &height);

    Theme::TextDialogData *_data;

    ActionEvent _ae; //!< Action event for event handling
    PWinObj *_text_wo;
    PDecor::TitleItem _title;

    // content related
    std::wstring _buf;
    uint _pos, _buf_off, _buf_chars; // position, start and num display

    // Completion
    std::wstring _buf_on_complete; /**< Buffer before completion. */
    std::wstring _buf_on_complete_result; /** Buffer after completion. */
    unsigned int _pos_on_complete; /**< Cursor position on completion start. */

    // history
    std::wstring _hist_new; // the one we started editing on
    std::vector<std::wstring> _history;
    std::vector<std::wstring>::iterator _hist_it;
    void addHistoryUnique(const std::wstring &);
    void loadHistory(const std::string &file);
    void saveHistory(const std::string &file);

private:
    static void addKeysymToKeysymMap(KeySym keysym, wchar_t chr);
    static std::map<KeySym, wchar_t> _keysym_map;
};
