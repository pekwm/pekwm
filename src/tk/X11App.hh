//
// X11App.hh for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_X11APP_HH_
#define _PEKWM_X11APP_HH_

#include "Os.hh"
#include "PWinObj.hh"
#include "X11.hh"

#include <set>

/**
 * Base for X11 applications
 */
class X11App : public PWinObj {
public:
	X11App(Geometry gm, int gm_mask, const std::string &title,
	       const char *wm_name, const char *wm_class,
	       AtomName window_type,
	       XSizeHints *normal_hints = nullptr,
	       bool double_buffer = false);
	virtual ~X11App(void);
	void stop(uint code);
	void addFd(int fd);
	void removeFd(int fd);

	virtual int main(uint timeout_s);

protected:
	bool hasBuffer(void) const;
	void setBackground(Pixmap pixmap);

	Drawable getRenderDrawable(void) const;
	Drawable getRenderBackground(void) const;

	virtual void handleEvent(XEvent*);
	virtual void handleFd(int);
	virtual void refresh(bool);
	virtual void swapBuffer(void);
	virtual void handleChildDone(pid_t, int);

private:
	virtual void screenChanged(const ScreenChangeNotification &scn);
	virtual void themeChanged(const std::string& name,
				  const std::string& variant, float scale) = 0;

	void themeChanged();
	void initSignalHandler(void);
	void handleSignal(void);
	bool waitForData(int timeout_s);

	void processEvent(void);

	std::string _wm_name;
	std::string _wm_class;
	XdbeBackBuffer _buffer;
	Pixmap _background;

	int _stop;
	int _dpy_fd;
	OsSelect *_select;
	std::set<Atom> _theme_atoms;
};

#endif // _PEKWM_X11APP_HH_
