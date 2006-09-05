//
// theme.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _THEME_HH_
#define _THEME_HH_

#include "baseconfig.hh"
#include "config.hh"
#include "screeninfo.hh"

#include "font.hh"
#include "button.hh"
#include "image.hh"

#include <string>
#include <vector>

// If adding, make sure it "syncs" with the windowmanager cursor enum
// BAD HABIT
enum BorderPosition {
	BORDER_TOP_LEFT, BORDER_TOP, BORDER_TOP_RIGHT,
	BORDER_LEFT, BORDER_RIGHT,
	BORDER_BOTTOM_LEFT, BORDER_BOTTOM, BORDER_BOTTOM_RIGHT,
	BORDER_NO_POS
};

class Theme
{
public:
	Theme(Config *c, ScreenInfo *s);
	~Theme();

	void load(void); // sets the actual theme up
	void setThemeDir(const std::string &dir);

	inline const GC &getInvertGC(void) const { return m_invert_gc; }

	// Window
	inline int getWinTitleHeight(void) const { return m_win_titleheight; }
	inline int getWinTitlePadding(void) const { return m_win_titlepadding; }

	inline PekFont *getWinFont(void) { return m_win_font; }
	inline unsigned int getWinFontJustify(void) const {
		return m_win_fontjustify; }

	// focused
	inline const XColor &getWinFocusedText(void) const {
		return m_win_focused_text; }
	inline Image* getWinFocusedPixmap(void) { return m_win_focused_pixmap; }
	inline Image* getWinFocusedSeparator(void) const {
		return m_win_focused_separator; }

	// unfocused
	inline const XColor &getWinUnfocusedText(void) const {
		return m_win_unfocused_text; }
	inline Image* getWinUnfocusedPixmap(void) { return m_win_unfocused_pixmap; }
	inline Image* getWinUnfocusedSeparator(void) const {
		return m_win_unfocused_separator; }

	// selected
	inline const XColor &getWinSelectedText(void) const {
		return m_win_selected_text; }
	inline Image* getWinSelectedPixmap(void) { return m_win_selected_pixmap; }

	inline std::list<FrameButton::ButtonData> *getButtonList(void) {
		return &m_button_list; }


	// Menu
	inline PekFont *getMenuFont(void) { return m_menu_font; }
	inline unsigned int getMenuPadding(void) const { return m_menu_padding; }
	inline unsigned int getMenuFontJustify(void) const {
		return m_menu_fontjustify; }

	inline const XColor &getMenuTextColor(void) const {
		return m_menu_textcolor; }
	inline const XColor &getMenuBackground(void) const {
		return m_menu_background; }
	inline const XColor &getMenuBackgroundSelected(void) const {
		return m_menu_backgroundselected; }
	inline const XColor &getMenuBorderColor(void) const {
		return m_menu_bordercolor; }

	inline unsigned int getMenuBorderWidth(void) const {
		return m_menu_borderwidth; }

#ifdef HARBOUR
	inline Image* getHarbourImage(void) const { return m_harbour_image; }
#endif // HARBOUR

	inline Image **getWinFocusedBorder(void) { return m_win_focused_border; }
	inline Image **getWinUnfocusedBorder(void) { return m_win_unfocused_border; }

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
	Image::ImageType getImageType(const std::string &type);

private:
	Config *cfg;
	ScreenInfo *scr;
	std::string m_theme_dir;

	bool m_is_loaded;

	GC m_invert_gc;

	// Window
	unsigned int m_win_titleheight;
	unsigned int m_win_titlepadding;

	PekFont *m_win_font;
	unsigned int m_win_fontjustify;

	// focused
	XColor m_win_focused_text;
	Image *m_win_focused_pixmap;
	Image *m_win_focused_separator;
	Image *m_win_focused_border[BORDER_NO_POS];

	// unfocused
	XColor m_win_unfocused_text;
	Image *m_win_unfocused_pixmap;
	Image *m_win_unfocused_separator;
	Image *m_win_unfocused_border[BORDER_NO_POS];

	// selected
	XColor m_win_selected_text;
	Image *m_win_selected_pixmap;

	std::list<FrameButton::ButtonData> m_button_list;

	// Harbour
#ifdef HARBOUR
	Image *m_harbour_image;
#endif // HARBOUR

	// Menu
	PekFont *m_menu_font;
	unsigned int m_menu_padding;
	unsigned int m_menu_fontjustify;

	XColor m_menu_textcolor, m_menu_background;
	XColor m_menu_backgroundselected, m_menu_bordercolor;

	unsigned int m_menu_borderwidth;

	// Lists
	struct justifylist_item {
		const char *name;
		TextJustify justify;
		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static justifylist_item m_justifylist[];

	struct pixmaptypelist_item {
		const char *name;
		Image::ImageType type;

		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static pixmaptypelist_item m_pixmaptype_list[];

};

#endif // _THEME_HH_
