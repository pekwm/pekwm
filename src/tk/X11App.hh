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

#ifdef PEKWM_X11_APP_MAIN

#include "Observable.hh"

#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "TextureHandler.hh"

#include <cassert>

static std::string _config_script_path;
static ObserverMapping *_observer_mapping = nullptr;
static FontHandler* _font_handler = nullptr;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}

	ObserverMapping* observerMapping()
	{
		assert(_observer_mapping);
		return _observer_mapping;
	}

	FontHandler* fontHandler()
	{
		assert(_font_handler);
		return _font_handler;
	}

	ImageHandler* imageHandler()
	{
		assert(_image_handler);
		return _image_handler;
	}

	TextureHandler* textureHandler()
	{
		assert(_texture_handler);
		return _texture_handler;
	}
}

static void
initX11App(Display* dpy, bool font_default_x11,
     const std::string &font_charset_override)
{
	_observer_mapping = new ObserverMapping();
	_font_handler =
		new FontHandler(font_default_x11, font_charset_override);
	_image_handler = new ImageHandler();
	_texture_handler = new TextureHandler();
}

static void
cleanupX11App()
{
	delete _texture_handler;
	delete _image_handler;
	delete _font_handler;
	delete _observer_mapping;
}

#endif // PEKWM_X11_APP_MAIN

#endif // _PEKWM_X11APP_HH_
