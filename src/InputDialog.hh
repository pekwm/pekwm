//
// CmdDialog.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _INPUT_DIALOG_HH_
#define _INPUT_DIALOG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <map>

extern "C" {
#include <X11/Xutil.h>
}

#include "PWinObj.hh"
#include "PWinObjReference.hh"
#include "PDecor.hh"
#include "Util.hh"

/**
 * Base for windows handling text input.
 */
class InputDialog : public PDecor, public PWinObjReference {
public:
  InputDialog(Display *dpy, Theme *theme, const std::wstring &title);
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

  virtual void mapCentered(const std::string &buf, bool focus, PWinObj *wo_ref = 0);
  virtual void moveCentered(PWinObj *wo);

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

  virtual void updateSize(void);
  virtual void updatePixmapSize(void);

    void getInputSize(unsigned int &width, unsigned int &height);

private:
    static void addKeysymToKeysymMap(KeySym keysym, wchar_t chr);

protected:
  Theme::TextDialogData *_data;

  ActionEvent _ae; //!< Action event for event handling
  PWinObj *_text_wo;
  PDecor::TitleItem _title;

  Pixmap _pixmap_bg;

  // content related
  std::wstring _buf;
  uint _pos, _buf_off, _buf_chars; // position, start and num display

    // Completion
    std::wstring _buf_on_complete; /**< Buffer before completion. */
    std::wstring _buf_on_complete_result; /** Buffer after completion. */
    unsigned int _pos_on_complete; /**< Cursor position on completion start. */

  // history
  std::wstring _hist_new; // the one we started editing on
  Util::file_backed_list _hist_list;
  Util::file_backed_list::iterator _hist_it;

private:
    static std::map<KeySym, wchar_t> _keysym_map;
};

#endif // _INPUT_DIALOG_HH_
