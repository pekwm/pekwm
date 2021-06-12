//
// InputDialog.hh for pekwm
// Copyright (C) 2009-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_INPUTDIALOG_HH_
#define _PEKWM_INPUTDIALOG_HH_

#include "config.h"

#include <map>

extern "C" {
#include <X11/Xlib.h>
}

#include "PWinObj.hh"
#include "PWinObjReference.hh"
#include "PDecor.hh"

class InputBuffer {
public:
	InputBuffer(void);
	InputBuffer(const std::string& buf, int pos=-1);
	~InputBuffer(void);

	const std::string& str(void) const { return _buf; }
	void setBuf(const std::string& str) { _buf = str; }
	uint pos(void) const { return _pos; }
	void setPos(uint pos) { _pos = pos; }

	size_t size(void) const { return _buf.size(); }

	void add(const std::string& str);
	void remove(void);
	void clear(void);
	void kill(void);
	void changePos(int off);

private:
	std::string _buf;
	uint _pos;
};

/**
 * Base for windows handling text input.
 */
class InputDialog : public PDecor,
                    public PWinObjReference {
public:
	InputDialog(const std::string &title);
	virtual ~InputDialog(void);

	// BEGIN - PWinObj interface
	virtual void mapWindow(void);

	// PWinObj event interface
	ActionEvent *handleButtonPress(XButtonEvent *ev);
	ActionEvent *handleKeyPress(XKeyEvent *ev);
	ActionEvent *handleExposeEvent(XExposeEvent *ev);
	// END - PWinObj interface

	void setTitle(const std::string &title);

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

	void bufClear(void);
	virtual void bufChanged(void);

	virtual void histNext(void);
	virtual void histPrev(void);

	virtual void updateSize(const Geometry &head);
	virtual void updatePixmapSize(void);

	ActionEvent& ae(void) { return _ae; }

	const std::string& str(void) const { return _buf.str(); }
	InputBuffer& buf(void) { return _buf; }

	void getInputSize(const Geometry &head, uint &width, uint &height);

	void addHistory(const std::string& entry, bool unique, uint max_size);
	void addHistoryUnique(const std::string& entry);
	void loadHistory(const std::string &file);
	void saveHistory(const std::string &file);

private:
	void bufAdd(XKeyEvent* ev);

protected:
	PWinObj *_text_wo;

private:
	Theme::TextDialogData *_data;

	PDecor::TitleItem _title;
	ActionEvent _ae; //!< Action event for event handling

	InputBuffer _buf;
	uint _buf_off;
	uint _buf_chars; // position, start and num display

	// history
	std::string _hist_new; // the one we started editing on
	std::vector<std::string> _history;
	std::vector<std::string>::iterator _hist_it;

	static void addKeysymToKeysymMap(KeySym keysym, const std::string& chr);
	static std::map<KeySym, std::string> _keysym_map;
};

#endif // _PEKWM_INPUTDIALOG_HH_
