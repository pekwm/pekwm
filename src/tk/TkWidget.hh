#ifndef _PEKWM_TK_WIDGET_HH_
#define _PEKWM_TK_WIDGET_HH_

#include "Types.hh"
#include "X11.hh"
#include "pekwm_types.hh"

#include "PSurface.hh"
#include "Render.hh"
#include "PWinObj.hh"
#include "Theme.hh"

typedef void(*stop_fun)(int);

class TkWidget {
public:
	virtual ~TkWidget(void)
	{
		if (_window != None) {
			X11::destroyWindow(_window);
		}
	}

	int getX(void) const { return _gm.x; }
	int getY(void) const { return _gm.y; }
	uint getHeight(void) const { return _gm.height; }
	void setHeight(uint height) { _gm.height = height; }

	virtual bool setState(Window, ButtonState) {
		return false;
	}
	virtual bool click(Window) { return false; }
	virtual void render(Render &rend, PSurface &surface) = 0;

	virtual void place(int x, int y, uint width, uint)
	{
		_gm.x = x;
		_gm.y = y;
		_gm.width = width;
	}

	/**
	 * Get requested width, 0 means adapt to given width.
	 */
	virtual uint widthReq(void) const { return 0; }

	/**
	 * Get requested height, given the provided width.
	 */
	virtual uint heightReq(uint width) const = 0;

protected:
	TkWidget(Theme::DialogData* data, PWinObj &parent)
		: _data(data),
		  _window(None),
		  _parent(parent)
	{
	}

	void setWindow(Window window) { _window = window; }

protected:
	Theme::DialogData *_data;
	Window _window;
	PWinObj &_parent;
	/** Widget geometry relative to dialog window */
	Geometry _gm;
};

#endif // _PEKWM_TK_WIDGET_HH_
