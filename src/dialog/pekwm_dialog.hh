//
// pekwm_dialog.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_DIALOG_HH_
#define _PEKWM_DIALOG_HH_

#include <string>

#include "../tk/PImage.hh"
#include "../tk/PPixmapSurface.hh"
#include "../tk/Theme.hh"
#include "../tk/TkWidget.hh"
#include "../tk/X11App.hh"

class PekwmDialogConfig {
public:
	PekwmDialogConfig(const std::string& title, PImage* image,
			  const std::string& message,
			  const std::vector<std::string>& options)
		: _title(title.empty() ? "pekwm_dialog" : title),
		  _image(image),
		  _message(message),
		  _options(options)
	{
		if (_options.empty()) {
			_options.push_back("Ok");
		}
	}

	const std::string& getTitle() const { return _title; }
	PImage* getImage() const { return _image; }
	const std::string& getMessage() const { return _message; }
	const std::vector<std::string>& getOptions() const { return _options; }

private:
	std::string _title;
	PImage* _image;
	std::string _message;
	std::vector<std::string> _options;
};

/**
 * Dialog
 *
 * --------------------------------------------
 * | TITLE                                    |
 * --------------------------------------------
 * | Image if any is displayed here           |
 * |                                          |
 * |                                          |
 * |                                          |
 * --------------------------------------------
 * | Message text goes here                   |
 * |                                          |
 * --------------------------------------------
 * |           [Option1] [Option2]            |
 * --------------------------------------------
 *
 */
class PekwmDialog : public X11App {
public:
	PekwmDialog(const std::string &theme_dir,
		    const std::string &theme_variant,
		    const Geometry &gm, int gm_mask, int decorations,
		    bool raise, const PekwmDialogConfig& config);
	virtual ~PekwmDialog();

	static PekwmDialog *instance() { return _instance; }

	virtual void handleEvent(XEvent *ev);
	virtual void resize(uint width, uint height);
	void show(void);
	void render();
	void setState(Window window, ButtonState state);
	void click(Window window);

protected:
	void initWidgets(const PekwmDialogConfig& config);
	void placeWidgets(void);

private:
	virtual void themeChanged(const std::string& name,
				  const std::string& variant, float scale);
	static void stopDialog(int retcode);

	Geometry _initial_gm;
	Theme _theme;
	Theme::DialogData* _data;
	bool _raise;

	PPixmapSurface _background;
	std::vector<TkWidget*> _widgets;

	static PekwmDialog *_instance;
};

#endif // _PEKWM_DIALOG_HH_
