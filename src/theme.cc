//
// theme.cc for pekwm
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

#include "pekwm.hh"
#include "config.hh"
#include "theme.hh"
#include "util.hh"

#include <X11/xpm.h>

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

const char *DEF_FONT = "fixed";

Theme::justifylist_item Theme::m_justifylist[] = {
	{"Left", LEFT_JUSTIFY},
	{"Center", CENTER_JUSTIFY},
	{"Right", RIGHT_JUSTIFY},
	{"", NO_JUSTIFY}
};

Theme::borderlist_item Theme::m_focused_borderlist[] = {
	{"border.top.focus", BORDER_TOP},
	{"border.left.focus", BORDER_LEFT},
	{"border.right.focus", BORDER_RIGHT},
	{"border.bottom.focus", BORDER_BOTTOM},

	{"border.top.left.focus", BORDER_TOP_LEFT},
	{"border.top.right.focus", BORDER_TOP_RIGHT},
	{"border.bottom.left.focus", BORDER_BOTTOM_LEFT},
	{"border.bottom.right.focus", BORDER_BOTTOM_RIGHT},
};

Theme::borderlist_item Theme::m_unfocused_borderlist[] = {
	{"border.top.unfocus", BORDER_TOP},
	{"border.left.unfocus", BORDER_LEFT},
	{"border.right.unfocus", BORDER_RIGHT},
	{"border.bottom.unfocus", BORDER_BOTTOM},

	{"border.top.left.unfocus", BORDER_TOP_LEFT},
	{"border.top.right.unfocus", BORDER_TOP_RIGHT},
	{"border.bottom.left.unfocus", BORDER_BOTTOM_LEFT},
	{"border.bottom.right.unfocus", BORDER_BOTTOM_RIGHT}
};

Theme::pixmaptypelist_item Theme::m_pixmaptype_list[] = {
	{"Tiled", Image::TILED},
	{"Scaled", Image::SCALED},
	{"Trans", Image::TRANSPARENT}
};

Theme::Theme(Config *c, ScreenInfo *s) :
cfg(c), scr(s),
m_win_font(NULL),
m_win_tj(LEFT_JUSTIFY),
m_win_bw(2), m_win_padding(2), m_win_title_height(15),
m_menu_font(NULL),
m_menu_tj(LEFT_JUSTIFY),
m_menu_bw(1), m_menu_padding(2),
m_is_loaded(false)
{
	XGrabServer(scr->getDisplay());

	m_theme_dir = cfg->getThemeDir();
	if (!m_theme_dir.size())
		return;

	if (m_theme_dir.at(m_theme_dir.size() - 1) != '/')
		m_theme_dir.append("/");

	m_win_font = new PekFont(scr);
	m_menu_font = new PekFont(scr);

	// just create empty pixmap holders
	m_win_image_fo = new Image(scr->getDisplay());
	m_win_image_un = new Image(scr->getDisplay());
	m_win_image_se = new Image(scr->getDisplay());
	m_win_separator_fo = new Image(scr->getDisplay());
	m_win_separator_un = new Image(scr->getDisplay());

	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		m_win_border_focus[i] = new Image(scr->getDisplay());
		m_win_border_unfocus[i] = new Image(scr->getDisplay());
	}

	loadTheme();

	XUngrabServer(scr->getDisplay());
}

Theme::~Theme()
{
	unloadTheme(); // should clean things up

	delete m_win_font;
	delete m_menu_font;

	delete m_win_image_fo;
	delete m_win_image_un;
	delete m_win_image_se;

	delete m_win_separator_fo;
	delete m_win_separator_un;

	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		delete m_win_border_focus[i];
		delete m_win_border_unfocus[i];
	}
}

void Theme::setThemeDir(const string &dir)
{
	m_theme_dir = dir;

	if (m_theme_dir.at(m_theme_dir.size() - 1) != '/')
		m_theme_dir.append("/");
}

//! @fn    void loadTheme(void)
//! @brief Loads the "ThemeFile", unloads any previous loaded theme.
void
Theme::loadTheme(void)
{
	string theme_file = m_theme_dir + string("theme");

	BaseConfig theme(theme_file, "*", ";");
	if (!theme.loadConfig()) {
		theme_file = DATADIR "/themes/default";
		theme.setFile(theme_file);

		if (! theme.loadConfig()) {
			cerr << "Couldn't load themedir: " << m_theme_dir
					 <<	" or default theme!" << endl;
			setupEmergencyTheme();
			return;
		}
	}

	if (m_is_loaded) {
		unloadTheme(); // unload last theme
	}
	loadButtons(&theme);

	Display *dpy = scr->getDisplay(); // convenience
	Colormap cl = scr->getColormap(); // convenience

	// temporary variables
	string s_value;
	XColor dummyc;
	XGCValues gv;

	// setup up window colors
	s_value.erase(); theme.getValue("win.text.focus", s_value);
	if (!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_win_text_fo, &dummyc)))
		m_win_text_fo.pixel = scr->getWhitePixel();
	s_value.erase(); theme.getValue("win.text.select", s_value);
	if (!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_win_text_se, &dummyc)))
		m_win_text_se.pixel = scr->getWhitePixel();
	s_value.erase(); theme.getValue("win.text.unfocus", s_value);
	if (!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_win_text_un, &dummyc)))
		m_win_text_un.pixel = scr->getBlackPixel();

	// setup menu colors
	s_value.erase(); theme.getValue("menu.text.color", s_value);
	if(!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_menu_text_c, &dummyc)))
		m_menu_text_c.pixel = scr->getWhitePixel();
	s_value.erase(); theme.getValue("menu.background", s_value);
	if(!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_menu_bg_c, &dummyc)))
		m_menu_bg_c.pixel = scr->getBlackPixel();
	s_value.erase(); theme.getValue("menu.select", s_value);
	if(!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_menu_select_c, &dummyc)))
		m_menu_select_c.pixel = scr->getBlackPixel();
	s_value.erase(); theme.getValue("menu.border.color", s_value);
	if(!(XAllocNamedColor(dpy, cl, s_value.c_str(), &m_menu_border_c, &dummyc)))
		m_menu_border_c.pixel = scr->getBlackPixel();

	// setup font's
	s_value.erase(); theme.getValue("win.text.font", s_value);
	m_win_font->load(s_value);

	s_value.erase(); theme.getValue("menu.text.font", s_value);
	m_menu_font->load(s_value);

	// int values
	theme.getValue("win.border.width", m_win_bw);
	if (m_win_bw > 25) m_win_bw = 25;
	theme.getValue("win.padding", m_win_padding);
	if (m_win_padding > 25) m_win_padding = 25;
	theme.getValue("win.title.height", m_win_title_height);
	if (m_win_title_height < (m_win_bw + (m_win_padding * 2)))
		m_win_title_height = m_win_bw + (m_win_padding * 2);


	if (theme.getValue("win.text.justify", s_value)) {
		m_win_tj = getTextJustify(s_value);
		if (m_win_tj == NO_JUSTIFY)
			m_win_tj = CENTER_JUSTIFY;
	} else {
		m_win_tj = CENTER_JUSTIFY;
	}

	if (theme.getValue("menu.text.justify", s_value)) {
		m_menu_tj = getTextJustify(s_value);
		if (m_menu_tj == NO_JUSTIFY)
			m_menu_tj = LEFT_JUSTIFY;
	} else {
		m_menu_tj = LEFT_JUSTIFY;
	}

	theme.getValue("menu.border.width", m_menu_bw);
	if (m_menu_bw > 25) m_menu_bw = 25;
	theme.getValue("menu.padding", m_menu_padding);
	if (m_menu_padding > 25) m_menu_padding = 25;

	loadBorderPixmaps(&theme);
	loadTitlebarPixmaps(&theme);

	// window gc's
	gv.function = GXinvert; 
	gv.foreground = m_win_text_fo.pixel;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 1;
	m_invert_gc = XCreateGC(dpy, scr->getRoot(),
		GCForeground|GCFunction|GCSubwindowMode|GCLineWidth, &gv);

	s_value.erase(); theme.getValue("root.command", s_value);
	
	if (s_value.length())
		Util::forkExec(s_value.c_str());

	m_is_loaded = true;
}

void Theme::setupEmergencyTheme(void)
{
	if (m_is_loaded)
		unloadTheme(); // unload last theme

	Display *dpy = scr->getDisplay(); // convenience

	// setup text colors
	m_win_text_fo.pixel = scr->getBlackPixel();
	m_win_text_se.pixel = scr->getBlackPixel();
	m_win_text_un.pixel = scr->getBlackPixel();

	m_win_font->load(DEF_FONT);

	// setup menu colors
	m_menu_text_c.pixel = scr->getWhitePixel();
	m_menu_bg_c.pixel = scr->getBlackPixel();
	m_menu_select_c.pixel = scr->getBlackPixel();
	m_menu_border_c.pixel = scr->getBlackPixel();

	m_menu_font->load(DEF_FONT);

	m_win_bw = 0;
	m_win_padding = 1;
	m_win_title_height = 15;

	m_win_tj = CENTER_JUSTIFY;

	m_menu_tj = LEFT_JUSTIFY;
	m_menu_bw = 2;
	m_menu_padding = 2;

	XGCValues gv;

	// window gc's
	gv.function = GXinvert; 
	gv.foreground = m_win_text_fo.pixel;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 1;
	m_invert_gc = XCreateGC(dpy, scr->getRoot(),
													GCForeground|GCFunction|GCSubwindowMode|GCLineWidth,
													&gv);

	m_is_loaded = true;
}

//! @fn    void unloadTheme(void)
//! @brief Unloads all pixmaps, fonts, gc and colors allocated by the theme.
void
Theme::unloadTheme(void) {
	// unload title buttons
	vector<FrameButton::ButtonData>::iterator b_it = m_button_list.begin();
	for (; b_it != m_button_list.end(); ++b_it) {
		for (unsigned int i = 0; i < FrameButton::BUTTON_NO_STATE; ++i) {
			XFreePixmap(scr->getDisplay(), b_it->pixmap[i]);
			XFreePixmap(scr->getDisplay(), b_it->shape[i]);
		}
	}
	m_button_list.clear();

	// unload title pixmaps
	m_win_image_fo->unload();
	m_win_image_un->unload();
	m_win_image_se->unload();

	m_win_separator_fo->unload();
	m_win_separator_un->unload();

	// unload border pixmaps
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		m_win_border_focus[i]->unload();
		m_win_border_unfocus[i]->unload();
	}

	m_win_font->unload();
	m_menu_font->unload();

	XFreeGC(scr->getDisplay(), m_invert_gc);

	unsigned long win_pixels[] =
		{m_win_text_fo.pixel, m_win_text_un.pixel, m_win_text_se.pixel};
	XFreeColors(scr->getDisplay(), scr->getColormap(), win_pixels, 3, 0);

	unsigned long menu_pixels[] =
		 {m_menu_text_c.pixel, m_menu_bg_c.pixel,
			m_menu_border_c.pixel, m_menu_select_c.pixel};
	XFreeColors(scr->getDisplay(), scr->getColormap(), menu_pixels, 4, 0);

	m_is_loaded = false;
}

TextJustify
Theme::getTextJustify(const string &justify)
{
	if (!justify.size())
		return NO_JUSTIFY;

	for (unsigned int i = 0; m_justifylist[i].justify != NO_JUSTIFY; ++i) {
		if (m_justifylist[i] == justify) {
			return m_justifylist[i].justify;
		}
	}

	return NO_JUSTIFY;
}

Image::ImageType
Theme::getImageType(const string &type)
{
	if (!type.size())
		return Image::TILED;

	for (unsigned int i = 0; m_pixmaptype_list[i].type; ++i) {
		if (m_pixmaptype_list[i] == type) {
			return m_pixmaptype_list[i].type;
		}
	}

	return Image::TILED;
}

//------------------------------ loadPixmap ---------------------------------
// Loads the Pixmap file and sets pix to the pixmap id and shape to the
// shaping id.
//---------------------------------------------------------------------------
bool
Theme::loadPixmap(const string &file, Pixmap &pix, Pixmap &shape,
									unsigned int &w, unsigned int &h)
{
	// Setup loading attributes attributes
	XpmAttributes attr;
	attr.valuemask = 0; attr.valuemask |= XpmVisual;
	attr.visual = scr->getVisual();

	if ((XpmReadFileToPixmap(scr->getDisplay(), scr->getRoot(),
													 (char *) file.c_str(), &pix, &shape, &attr))
			 != XpmSuccess) {

		pix = None;
		shape = None;
		w = h = 0;

		return false;
	}

	w = attr.width;
	h = attr.height;

	return true;
}

//! @fn    void loadBorderPixmaps(BaseConfig *theme)
void
Theme::loadBorderPixmaps(BaseConfig *theme)
{
	string file;

	// load focused pixmaps
	for (int i = 0; i < BORDER_NO_POS; ++i) {
		if (theme->getValue(m_focused_borderlist[i].name, file)) {
			loadImage(m_win_border_focus[i], m_theme_dir + file);
		}
	}

	// load unfocused pixmaps
	for (int i = 0; i < BORDER_NO_POS; ++i) {
		if (theme->getValue(m_unfocused_borderlist[i].name, file)) {
			loadImage(m_win_border_unfocus[i], m_theme_dir + file);
		}
	}
}

//! @fn    void loadButtons(BaseConfig *theme)
void
Theme::loadButtons(BaseConfig *theme)
{
	if (! theme)
		return;

	vector<string> actions, pixmaps, tmp;
	vector<string>::iterator it;

	string value;
	bool left;
	while (true) {
		value.erase();
		if (theme->getValue("win.button.right", value)) {
			left = false;
		} else if (theme->getValue("win.button.left", value)) {
			left = true;
		} else {
			break; // no more buttons left
		}

		if (tmp.size())
			tmp.clear();

		vector<string>::iterator a;
		if ((Util::splitString(value, tmp, ":", 2)) == 2) {
			a = tmp.begin();
		} else {
			continue; // not a valid line
		}

		FrameButton::ButtonData button;

		if (actions.size())
			actions.clear();

		unsigned int num_actions = Util::splitString(*a, actions, " \t", 3);
		if (num_actions < 1)
			continue; // not a valid line

		it = actions.begin();
		for (unsigned int i = 0; i < 3; ++i, ++it) {
			if (num_actions > i) {
				button.action[i].action = cfg->getAction(*it, BUTTONCLICK_OK);
			} else {
				button.action[i].action = NO_ACTION;
			}
		}

		if (pixmaps.size())
			pixmaps.clear();

		// load the button pixmaps
		if (Util::splitString(*(++a), pixmaps, " \t", 3) == 3) {
			it = pixmaps.begin();
			unsigned int w = 0, h = 0;
			for (unsigned int i = 0; it != pixmaps.end(); ++i, ++it) {
				loadPixmap(m_theme_dir + *it, button.pixmap[i],
									 button.shape[i], w, h);
			}

			button.width = w;
			button.height = h;

			button.left = left;

			m_button_list.push_back(button);
		}
	}
}

//------------------------------ loadPekPixmap ------------------------------
//---------------------------------------------------------------------------
void
Theme::loadImage(Image *image,
								 const string &file, const string &type)
{
	if (!image || !file.size())
		return;

	Image::ImageType image_type = Image::TILED;
	if (type.size()) {
		image_type = getImageType(type);
	}

	image->load(file);
	image->setImageType(image_type);
}

//--------------------------- loadTitlebarPixmaps ---------------------------
//---------------------------------------------------------------------------
void
Theme::loadTitlebarPixmaps(BaseConfig *theme)
{
	string s_value;
	vector<string> tokens;

	if (theme->getValue("win.focus.pixmap", s_value)) {
		if ((Util::splitString(s_value, tokens, ":", 2)) == 2) {
			loadImage(m_win_image_fo, m_theme_dir + tokens[0], tokens[1]);
		} else {
			loadImage(m_win_image_fo, m_theme_dir + s_value);
		}
	}

	tokens.clear();
	if (theme->getValue("win.unfocus.pixmap", s_value)) {
		if ((Util::splitString(s_value, tokens, ":", 2)) == 2) {
			loadImage(m_win_image_un, m_theme_dir + tokens[0], tokens[1]);
		} else {
			loadImage(m_win_image_un, m_theme_dir + s_value);
		}
	}

	tokens.clear();
	if (theme->getValue("win.select.pixmap", s_value)) {
		if ((Util::splitString(s_value, tokens, ":", 2)) == 2) {
			loadImage(m_win_image_se, m_theme_dir + tokens[0], tokens[1]);
		} else {
			loadImage(m_win_image_se, m_theme_dir + s_value);
		}
	}

	tokens.clear();
	if (theme->getValue("win.focused.separator", s_value)) {
		if ((Util::splitString(s_value, tokens, ":", 2)) == 2) {
			loadImage(m_win_separator_fo, m_theme_dir + tokens[0], tokens[1]);
		} else {
			loadImage(m_win_separator_fo, m_theme_dir + s_value);
		}
	}

	tokens.clear();
	if (theme->getValue("win.unfocused.separator", s_value)) {
		if ((Util::splitString(s_value, tokens, ":", 2)) == 2) {
			loadImage(m_win_separator_un, m_theme_dir + tokens[0], tokens[1]);
		} else {
			loadImage(m_win_separator_un, m_theme_dir + s_value);
		}
	}
}
