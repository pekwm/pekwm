//
// Theme.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _THEME_HH_
#define _THEME_HH_

#include "pekwm.hh"
#include "BaseConfig.hh"

class ScreenInfo;
class Config;
class Button;
class ButtonData;
class PekwmFont;
class Image;

#include <string>
#include <vector>

class Theme
{
public:
	Theme(ScreenInfo *s, Config *c);
	~Theme();

	void load(void); // sets the actual theme up
	void setThemeDir(const std::string& dir);

	inline const GC &getInvertGC(void) const { return _invert_gc; }

	// Window
	inline int getWinTitleHeight(void) const { return _win_titleheight; }
	inline int getWinTitlePadding(void) const { return _win_titlepadding; }

	inline PekwmFont *getWinFont(void) { return _win_font; }
	inline unsigned int getWinFontJustify(void) const {
		return _win_fontjustify; }

	// titlebar
	inline Image* getWinImageFo(void) { return _win_image_fo; }
	inline Image* getWinImageFoSe(void) { return _win_image_fose; }
	inline Image* getWinImageUn(void) { return _win_image_un; }
	inline Image* getWinImageUnSe(void) { return _win_image_unse; }

	// titlebar separator
	inline Image* getWinSepFo(void) { return _win_sep_fo; }
	inline Image* getWinSepUn(void) { return _win_sep_un; }

	// titlebar text
	inline const XColor &getWinTextFo(void) const { return _win_text_fo; }
	inline const XColor &getWinTextFoSe(void) const { return _win_text_fose; }
	inline const XColor &getWinTextUn(void) const { return _win_text_un; }
	inline const XColor &getWinTextUnSe(void) const { return _win_text_unse; }	

	// titlebar buttons
	inline std::list<ButtonData*> *getButtonList(void) { return &_button_list; }

	// frame border
	inline Image **getWinFocusedBorder(void) { return _win_focused_border; }
	inline Image **getWinUnfocusedBorder(void) { return _win_unfocused_border; }

	// Menu
	inline PekwmFont *getMenuFont(void) { return _menu_font; }
	inline unsigned int getMenuPadding(void) const { return _menu_padding; }
	inline unsigned int getMenuFontJustify(void) const {
		return _menu_fontjustify; }

	inline const XColor &getMenuTextColor(void) const {
		return _menu_text; }
	inline const XColor &getMenuTextColorSe(void) const {
		return _menu_text_se; }
	inline const XColor &getMenuTextColorTi(void) const {
		return _menu_text_ti; }
	inline const XColor &getMenuBackground(void) const {
		return _menu_background; }
	inline const XColor &getMenuBackgroundSe(void) const {
		return _menu_background_se; }
	inline const XColor &getMenuBackgroundTi(void) const {
		return _menu_background_ti; }
	inline const XColor &getMenuBorderColor(void) const {
		return _menu_border; }

	inline unsigned int getMenuBorderWidth(void) const {
		return _menu_borderwidth; }

#ifdef HARBOUR
	inline Image* getHarbourImage(void) const { return _harbour_image; }
#endif // HARBOUR

private:
	void unload(void);
	void setupEmergencyTheme(void); // if we can't open a theme

	bool loadXpm(const std::string &file, Pixmap &pix, Pixmap &shape,
							 unsigned int &width, unsigned int &height);
	void loadImage(Image *image, const std::string &file);
	bool allocColor(const std::string &color, XColor *xcolor);

	void loadBorder(BaseConfig::CfgSection *cs, Image **data);
	void loadButtons(BaseConfig::CfgSection *cs);

	TextJustify getFontJustify(const std::string &type);
	ImageType getImageType(const std::string &type);

private:
	ScreenInfo *_scr;
	Config *_cfg;
	std::string _theme_dir;

	bool _is_loaded;

	GC _invert_gc;

	// Window
	unsigned int _win_titleheight;
	unsigned int _win_titlepadding;

	PekwmFont *_win_font;
	unsigned int _win_fontjustify;

	// titlebar
	Image *_win_image_fo, *_win_image_fose;
	Image *_win_image_un, *_win_image_unse;

	// titlebar separator
	Image *_win_sep_fo, *_win_sep_un;

	// titelbar text
	XColor _win_text_fo, _win_text_fose;
	XColor _win_text_un, _win_text_unse;

	// titlebar buttons
	std::list<ButtonData*> _button_list;

	// frame border
	Image *_win_focused_border[BORDER_NO_POS];
	Image *_win_unfocused_border[BORDER_NO_POS];

	// Harbour
#ifdef HARBOUR
	Image *_harbour_image;
#endif // HARBOUR

	// Menu
	PekwmFont *_menu_font;
	unsigned int _menu_padding;
	unsigned int _menu_fontjustify;

	XColor _menu_text, _menu_text_se, _menu_text_ti;
	XColor _menu_background, _menu_background_se, _menu_background_ti;
	XColor _menu_border;

	unsigned int _menu_borderwidth;

	struct UnsignedListItem {
		const char *name;
		unsigned int value;
		inline bool operator == (const std::string& s) {
			return !strcasecmp(name, s.c_str());
		}
	};
	static UnsignedListItem _justifylist[];
	static UnsignedListItem _pixmaptype_list[];
};

#endif // _THEME_HH_
