//
// PanelTheme.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_PANEL_THEME_HH_
#define _PEKWM_PANEL_PANEL_THEME_HH_

#include <string>

#include "pekwm_panel.hh"
#include "CfgParser.hh"
#include "X11.hh"

#include "../tk/PFont.hh"
#include "../tk/PTexture.hh"

/**
 * Theme for the panel and its widgets.
 */
class PanelTheme {
public:
	PanelTheme();
	~PanelTheme();

	void load(const std::string &theme_dir, const std::string& theme_path);
	void unload();

	void setIconPath(const std::string& config_path,
			 const std::string& theme_path);

	uint getHeight(void) const { return _height; }

	PFont *getFont(ClientState state) const;
	PTexture *getBackground(void) const { return _background; }
	PTexture *getSep(void) const { return _sep; }
	PTexture *getHandle(void) const { return _handle; }

	// Bar specific themeing
	XColor *getBarBorder(void) const { return _bar_border; }
	XColor *getBarFill(void) const { return _bar_fill; }

private:
	void loadState(CfgParser::Entry *section, ClientState state);
	void check(void);

	/** Panel height, can be fixed or calculated from font size */
	uint _height;
	/** Set to true after load has been called. */
	bool _loaded;

	/** Panel background texture. */
	PTexture *_background;
	/** If < 255 (dervied from 0-100%) panel background is blended on
	    top of background image. */
	uchar _background_opacity;
	/** Texture rendered between widgets in the bar. */
	PTexture *_sep;
	/** Texture rendered at the left and right of the bar, optional. */
	PTexture *_handle;
	PFont* _fonts[CLIENT_STATE_NO];
	PFont::Color* _colors[CLIENT_STATE_NO];

	/** Bar border color. */
	XColor *_bar_border;
	/** Bar fill color. */
	XColor *_bar_fill;
};


#endif // _PEKWM_PANEL_PANEL_THEME_HH_
