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
	BORDER_TOP,
	BORDER_LEFT, BORDER_RIGHT,
	BORDER_BOTTOM,
	BORDER_TOP_LEFT, BORDER_TOP_RIGHT,
	BORDER_BOTTOM_LEFT, BORDER_BOTTOM_RIGHT,
	BORDER_NO_POS
};

class Theme
{
public:
	Theme(Config *c, ScreenInfo *s);
	~Theme();

	void loadTheme(void); // sets the actual theme up
	void setThemeDir(const std::string &dir);

	inline std::vector<FrameButton::ButtonData> *getButtonList(void) {
		return &m_button_list; }

	inline const GC &getInvertGC(void) const { return m_invert_gc; }

	inline const XColor &getWinTextFo(void) const { return m_win_text_fo; }
	inline const XColor &getWinTextUn(void) const { return m_win_text_un; }
	inline const XColor &getWinTextSe(void) const { return m_win_text_se; }

	inline PekFont *getWinFont(void) { return m_win_font; }
	// used for status window too
	inline PekFont *getMenuFont(void) { return m_menu_font; }

	inline int getWinBW(void) const { return m_win_bw; }
	inline int getWinPadding(void) const { return m_win_padding; }
	inline int getWinTitleHeight(void) const { return m_win_title_height; }
	inline int getWinTextJustify(void) const { return m_win_tj; }

	inline Image* getWinFocusImage(void) const {
		return m_win_image_fo; }
	inline Image* getWinUnfocusImage(void) const {
		return m_win_image_un; }
	inline Image* getWinSelectImage(void) const {
		return m_win_image_se; }

	inline Image* getWinFocusSeparator(void) const {
		return m_win_separator_fo; }
	inline Image* getWinUnfocusSeparator(void) const {
		return m_win_separator_un;
	}

	inline Image **getBorderFocusData(void) {
		return m_win_border_focus; }
	inline Image **getBorderUnfocusData(void) {
		return m_win_border_unfocus; }

	inline int getMenuBW(void) const { return m_menu_bw; }
	inline int getMenuPadding(void) const { return m_menu_padding; }
	inline const int &getMenuTextJustify(void) const { return m_menu_tj; }

	inline const XColor &getMenuTextC(void) const { return m_menu_text_c; }
	inline const XColor &getMenuSelectC(void) const { return m_menu_select_c; }
	inline const XColor &getMenuBackgroundC(void) const { return m_menu_bg_c; }
	inline const XColor &getMenuBorderC(void) const { return m_menu_border_c; }

private:
	void setupEmergencyTheme(void); // if we can't open a theme

	void unloadTheme(void);
	bool loadPixmap(const std::string &file, Pixmap &pix, Pixmap &shape,
									unsigned int &w, unsigned int &h);
	void loadImage(Image *image,
								 const std::string &file, const std::string &type = "");

	void loadBorderPixmaps(BaseConfig *theme);
	void loadButtons(BaseConfig *theme);
	void loadTitlebarPixmaps(BaseConfig *theme);

	TextJustify getTextJustify(const std::string &type);
	Image::ImageType getImageType(const std::string &type);


private:
	Config *cfg;
	ScreenInfo *scr;
	std::string m_theme_dir;

	struct justifylist_item {
		const char *name;
		TextJustify justify;
		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static justifylist_item m_justifylist[];

	struct borderlist_item {
		std::string name;
		BorderPosition position;
	};
	static borderlist_item m_focused_borderlist[];
	static borderlist_item m_unfocused_borderlist[];

	struct pixmaptypelist_item {
		const char *name;
		Image::ImageType type;

		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static pixmaptypelist_item m_pixmaptype_list[];

	// window stuff
	PekFont *m_win_font;
	GC m_invert_gc;

	XColor m_win_text_fo, m_win_text_un, m_win_text_se;

	std::vector<FrameButton::ButtonData> m_button_list;

	Image *m_win_image_fo, *m_win_image_un, *m_win_image_se;
	Image *m_win_separator_fo, *m_win_separator_un;

	Image *m_win_border_focus[BORDER_NO_POS];
	Image *m_win_border_unfocus[BORDER_NO_POS];

	int m_win_tj;
	unsigned int m_win_bw, m_win_padding, m_win_title_height;

	PekFont *m_menu_font;
	XColor m_menu_text_c, m_menu_bg_c, m_menu_border_c, m_menu_select_c;

	int m_menu_tj;
	unsigned int m_menu_bw, m_menu_padding;

	bool m_is_loaded;
};

#endif // _THEME_HH_
