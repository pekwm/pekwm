#ifndef _PEKWM_SHOW_HH_
#define _PEKWM_SHOW_HH_

#include "../tk/TkLayout.hh"
#include "../tk/PImage.hh"
#include "../tk/PPixmapSurface.hh"
#include "../tk/Theme.hh"
#include "../tk/X11App.hh"

#include <string>
#include <vector>

class PekwmShow : public X11App {
public:
	PekwmShow(Theme::DialogData* data,
		  const Geometry &gm, int gm_mask,
		  bool raise, const std::vector<std::string>& images,
		  const std::string& title, PImage* image);
	virtual ~PekwmShow();

	static PekwmShow *instance(void) { return _instance; }

	virtual void handleEvent(XEvent *ev);
	virtual void resize(uint width, uint height);

	void show();

protected:
	void initWidgets(const std::string& title, PImage* image);

private:
	void render();
	void setState(Window window, ButtonState state);
	void click(Window window, int button);

	static void stopShow(int retcode);

private:
	Theme::DialogData* _data;
	bool _raise;
	std::vector<std::string> _images;

	PPixmapSurface _background;
	TkLayout _layout;
	std::vector<TkWidget*> _widgets;

	static PekwmShow *_instance;
};

#endif // _PEKWM_SHOW_HH_
