//
// X11App.hh for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_X11APP_HH_
#define _PEKWM_X11APP_HH_

#include "PWinObj.hh"
#include "X11.hh"

/**
 * Base for X11 applications
 */
class X11App : public PWinObj {
public:
	X11App(Geometry gm, const std::string &title,
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

	virtual void screenChanged(const ScreenChangeNotification &scn);

private:
	void initSignalHandler(void);
	void handleSignal(void);
	bool waitForData(int timeout_s);

	void processEvent(void);

private:
	std::string _wm_name;
	std::string _wm_class;
	XdbeBackBuffer _buffer;
	Pixmap _background;

	int _stop;
	std::vector<int> _fds;
	int _dpy_fd;
	int _max_fd;
};

#endif // _PEKWM_X11APP_HH_
