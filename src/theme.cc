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
using std::list;
using std::vector;

const char *DEF_FONT = "fixed";

Theme::justifylist_item Theme::m_justifylist[] = {
	{"LEFT", LEFT_JUSTIFY},
	{"CENTER", CENTER_JUSTIFY},
	{"RIGHT", RIGHT_JUSTIFY},
	{"", NO_JUSTIFY}
};

Theme::pixmaptypelist_item Theme::m_pixmaptype_list[] = {
	{"TILED", Image::TILED},
	{"SCALED", Image::SCALED},
	{"TRANS", Image::TRANSPARENT}
};

Theme::Theme(Config *c, ScreenInfo *s) :
cfg(c), scr(s),
m_is_loaded(false),
m_invert_gc(None),
m_win_titleheight(15), m_win_titlepadding(1),
m_win_fontjustify(CENTER_JUSTIFY),
m_menu_padding(2), m_menu_fontjustify(LEFT_JUSTIFY),
m_menu_borderwidth(1)
{
	// window gc's
	XGCValues gv;
	gv.function = GXinvert;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 1;
	m_invert_gc = XCreateGC(scr->getDisplay(), scr->getRoot(),
													GCFunction|GCSubwindowMode|GCLineWidth, &gv);

	// Window
	m_win_font = new PekFont(scr);

	// focus
	m_win_focused_pixmap = new Image(scr->getDisplay());
	m_win_focused_separator = new Image(scr->getDisplay());
	// unfocus
	m_win_unfocused_pixmap = new Image(scr->getDisplay());
	m_win_unfocused_separator = new Image(scr->getDisplay());
	// selected
	m_win_selected_pixmap = new Image(scr->getDisplay());
	// borders
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		m_win_focused_border[i] = new Image(scr->getDisplay());
		m_win_unfocused_border[i] = new Image(scr->getDisplay());
	}

	// Harbour
#ifdef HARBOUR
	m_harbour_image = new Image(scr->getDisplay());
#endif // HARBOUR

	// Menu
	m_menu_font = new PekFont(scr);

	XGrabServer(scr->getDisplay());

	setThemeDir(cfg->getThemeFile());
	load();

	XSync(scr->getDisplay(), false);
	XUngrabServer(scr->getDisplay());
}

Theme::~Theme()
{
	unload(); // should clean things up

	XFreeGC(scr->getDisplay(), m_invert_gc);

	delete m_win_font;

	// focus
	delete m_win_focused_pixmap;
	delete m_win_focused_separator;
	// unfocus
	delete m_win_unfocused_pixmap;
	delete m_win_unfocused_separator;
	// selected
	delete m_win_selected_pixmap;
	// borders
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		delete m_win_focused_border[i];
		delete m_win_unfocused_border[i];
	}

#ifdef HARBOUR
	delete m_harbour_image;
#endif // HARBOUR

	delete m_menu_font;
}

//! @fn    void setThemeDir(const string &dir)
//! @bried Sets the path to the theme, make sure's there's an ending /
void
Theme::setThemeDir(const string &dir)
{
	if (!dir.size())
		return;

	m_theme_dir = dir;

	if (m_theme_dir.at(m_theme_dir.size() - 1) != '/')
		m_theme_dir.append("/");
}

//! @fn    void load(void)
//! @brief Loads the "ThemeFile", unloads any previous loaded theme.
void
Theme::load(void)
{
	if (m_is_loaded)
		unload();

	BaseConfig theme;
	BaseConfig::CfgSection *cs, *sect;

	string theme_file = m_theme_dir + string("theme");
	if (!theme.load(theme_file)) {
		theme_file = DATADIR "/themes/default";
		if (!theme.load(theme_file)) {
			cerr << "Couldn't load themedir: " << m_theme_dir
					 <<	" or default theme!" << endl;
			setupEmergencyTheme();
			return;
		}
	}

	// temporary variables used in parsing
	string s_value;

	if ((cs = theme.getSection("WINDOW"))) {
		cs->getValue("TITLEHEIGHT", m_win_titleheight);
		cs->getValue("TITLEPADDING", m_win_titlepadding);

		if ((sect = cs->getSection("FONT"))) {
			sect->getValue("NAME", s_value);
			m_win_font->load(s_value);
			sect->getValue("JUSTIFY", s_value);
			m_win_fontjustify = getFontJustify(s_value);
			if (m_win_fontjustify == NO_JUSTIFY)
				m_win_fontjustify = LEFT_JUSTIFY;
		}

		if ((sect = cs->getSection("FOCUSED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &m_win_focused_text))
				m_win_focused_text.pixel = scr->getBlackPixel();

			if (sect->getValue("PIXMAP", s_value))
				loadImage(m_win_focused_pixmap, m_theme_dir + s_value);
			if (sect->getValue("SEPARATOR", s_value))
				loadImage(m_win_focused_separator, m_theme_dir + s_value);

			loadBorder(sect->getSection("BORDER"), m_win_focused_border);
		}

		if ((sect = cs->getSection("UNFOCUSED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &m_win_unfocused_text))
				m_win_unfocused_text.pixel = scr->getBlackPixel();

			if (sect->getValue("PIXMAP", s_value))
				loadImage(m_win_unfocused_pixmap, m_theme_dir + s_value);
			if (sect->getValue("SEPARATOR", s_value))
				loadImage(m_win_unfocused_separator, m_theme_dir + s_value);

			loadBorder(sect->getSection("BORDER"), m_win_unfocused_border);
		}

		if ((sect = cs->getSection("SELECTED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &m_win_selected_text))
				m_win_selected_text.pixel = scr->getWhitePixel();
			if (sect->getValue("PIXMAP", s_value))
				loadImage(m_win_selected_pixmap, m_theme_dir + s_value);
		}

		if ((sect = cs->getSection("BUTTONS")))
			loadButtons(sect);
	}

	if ((cs = theme.getSection("MENU"))) {
		cs->getValue("FONT", s_value);
		m_menu_font->load(s_value);
		cs->getValue("PADDING", m_menu_padding);
		cs->getValue("TEXTJUSTIFY", s_value);
		m_menu_fontjustify = getFontJustify(s_value);
		if (m_menu_fontjustify == NO_JUSTIFY)
			m_menu_fontjustify = LEFT_JUSTIFY;

		// Load colors
		cs->getValue("TEXTCOLOR", s_value);
		if (!allocColor(s_value, &m_menu_textcolor))
			m_menu_textcolor.pixel = scr->getWhitePixel();
		cs->getValue("BACKGROUND", s_value);
		if (!allocColor(s_value, &m_menu_background))
			m_menu_background.pixel = scr->getBlackPixel();
		cs->getValue("BACKGROUNDSELECTED", s_value);
		if (!allocColor(s_value, &m_menu_backgroundselected))
			m_menu_backgroundselected.pixel = scr->getWhitePixel();
		cs->getValue("BORDERCOLOR", s_value);
		if (!allocColor(s_value, &m_menu_bordercolor))
			m_menu_bordercolor.pixel = scr->getBlackPixel();
		cs->getValue("BORDERWIDTH", m_menu_borderwidth);

		// Set the color of the font
		m_menu_font->setColor(m_menu_textcolor);
	}

	if ((cs = theme.getSection("ROOT"))) {
		if (cs->getValue("COMMAND", s_value))
			Util::forkExec(s_value.c_str());
	}


#ifdef HARBOUR
	if ((cs = theme.getSection("HARBOUR"))) {
		if (cs->getValue("PIXMAP", s_value))
			loadImage(m_harbour_image, m_theme_dir + s_value);
	}
#endif // HARBOUR

	m_is_loaded = true;
}

//! @fn    void setupEmergencyTheme(void)
//! @brief Sets a theme up incase no theme can be found.
void
Theme::setupEmergencyTheme(void)
{
	if (m_is_loaded)
		unload(); // unload last theme

	// Window
	m_win_titleheight = 15;
	m_win_titlepadding = 1;
	m_win_font->load(DEF_FONT);
	m_win_fontjustify = CENTER_JUSTIFY;

	m_win_focused_text.pixel = scr->getBlackPixel();
	m_win_unfocused_text.pixel = scr->getBlackPixel();
	m_win_selected_text.pixel = scr->getBlackPixel();

	// setup menu colors
	m_menu_font->load(DEF_FONT);
	m_menu_padding = 2;
	m_menu_fontjustify = LEFT_JUSTIFY;

	m_menu_textcolor.pixel = scr->getWhitePixel();
	m_menu_background.pixel = scr->getBlackPixel();
	m_menu_backgroundselected.pixel = scr->getBlackPixel();
	m_menu_bordercolor.pixel = scr->getBlackPixel();

	m_is_loaded = true;
}

//! @fn    void unload(void)
//! @brief Unloads all pixmaps, fonts, gc and colors allocated by the theme.
void
Theme::unload(void) {
	// unload title buttons
	list<FrameButton::ButtonData>::iterator b_it = m_button_list.begin();
	for (; b_it != m_button_list.end(); ++b_it) {
		for (unsigned int i = 0; i < FrameButton::BUTTON_NO_STATE; ++i) {
			XFreePixmap(scr->getDisplay(), b_it->pixmap[i]);
			XFreePixmap(scr->getDisplay(), b_it->shape[i]);
		}
	}
	m_button_list.clear();

	m_win_font->unload();

	// unload title pixmaps
	m_win_focused_pixmap->unload();
	m_win_focused_separator->unload();
	m_win_unfocused_pixmap->unload();
	m_win_unfocused_separator->unload();
	m_win_selected_pixmap->unload();

	// unload border pixmaps
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		m_win_focused_border[i]->unload();
		m_win_unfocused_border[i]->unload();
	}

	// free window pixels
	unsigned long win_pixels[] =
		{m_win_focused_text.pixel, m_win_unfocused_text.pixel,
		 m_win_selected_text.pixel};
	XFreeColors(scr->getDisplay(), scr->getColormap(), win_pixels, 3, 0);

#ifdef HARBOUR
	m_harbour_image->unload();
#endif // HARBOUR

	// unload menu
	m_menu_font->unload();
	unsigned long menu_pixels[] =
		 {m_menu_textcolor.pixel, m_menu_background.pixel,
			m_menu_backgroundselected.pixel, m_menu_bordercolor.pixel};
	XFreeColors(scr->getDisplay(), scr->getColormap(), menu_pixels, 4, 0);



	m_is_loaded = false;
}

//! @fn    TextJustify getFontJustify(const string &justify)
//! @brief
TextJustify
Theme::getFontJustify(const string &justify)
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

//! @fn    Image::ImageType getImageType(const string &type)
//! @brief
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

//! @fn    void loadBorder(BaseConfig::CfgSection *cs, Image **data)
//! @brief Loads a Border section and fills the data array.
void
Theme::loadBorder(BaseConfig::CfgSection *cs, Image **data)
{
	if (!cs)
		return;

	string s_value;
	vector<string> tokens;

	if (cs->getValue("TOP", s_value)) {
		if ((Util::splitString(s_value, tokens, " \t", 3)) == 3) {
			data[BORDER_TOP_LEFT]->load(m_theme_dir + tokens[0]);
			data[BORDER_TOP]->load(m_theme_dir + tokens[1]);
			data[BORDER_TOP_RIGHT]->load(m_theme_dir + tokens[2]);
		}
		tokens.clear();
	}

	if (cs->getValue("SIDE", s_value)) {
		if ((Util::splitString(s_value, tokens, " \t", 2)) == 2) {
			data[BORDER_LEFT]->load(m_theme_dir + tokens[0]);
			data[BORDER_RIGHT]->load(m_theme_dir + tokens[1]);
		}
		tokens.clear();
	}

	if (cs->getValue("BOTTOM", s_value)) {
		if ((Util::splitString(s_value, tokens, " \t", 3)) == 3) {
			data[BORDER_BOTTOM_LEFT]->load(m_theme_dir + tokens[0]);
			data[BORDER_BOTTOM]->load(m_theme_dir + tokens[1]);
			data[BORDER_BOTTOM_RIGHT]->load(m_theme_dir + tokens[2]);
		}
	}
}

//! @fn    void loadButtons(BaseConfig::CfgSection *cs)
//! @brief Loads the Titlebar buttons.
void
Theme::loadButtons(BaseConfig::CfgSection *cs)
{
	if (!cs)
		return;

	BaseConfig::CfgSection *sect;

	FrameButton::ButtonData button;
	string s_value;
	vector<string> tokens;
	vector<string>::iterator it;

	while ((sect = cs->getNextSection())) {
		if (*sect == "LEFT") {
			button.left = true;
		} else if (*sect == "RIGHT") {
			button.left = false;
		} else
			continue; // not a button section

		if (!sect->getValue("ACTIONS", s_value))
			continue; // we need a action

		tokens.clear();
		unsigned int num_actions =
			Util::splitString(s_value, tokens, " \t", FrameButton::NUM_BUTTONS);

		for (unsigned int i = 0; i < FrameButton::NUM_BUTTONS; ++i)
			button.action[i].action = NO_ACTION;

		if (num_actions) {
			it = tokens.begin();
			for (unsigned int i = 0; i < num_actions; ++i, ++it) {
				button.action[i].action = cfg->getAction(*it, BUTTONCLICK_OK);
			}
		} else {
			button.action[0].action = cfg->getAction(s_value, BUTTONCLICK_OK);
		}

		if (!sect->getValue("PIXMAPS", s_value))
			continue; // we need pixmaps

		tokens.clear();
		if (Util::splitString(s_value, tokens, " \t", 3) == 3) {
			unsigned int w = 0, h = 0;

			it = tokens.begin();
			for (unsigned int i = 0; it != tokens.end(); ++i, ++it) {
				loadXpm(m_theme_dir + *it, button.pixmap[i], button.shape[i], w, h);
			}

			button.width = w;
			button.height = h;

			m_button_list.push_back(button);
		}
	}
}


//! @fn    void loadXpm(const string &file, Pixmap &pix, Pixmap &shape, unsigned int &width, unsgined int &height)
//! @brief Loads an Xpm image and stores it in pix and shape.
bool
Theme::loadXpm(const string &file, Pixmap &pix, Pixmap &shape,
							 unsigned int &width, unsigned int &height)
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
		width = height = 0;

		return false;
	}

	width = attr.width;
	height = attr.height;

	return true;
}


//! @fn    void loadImage(Image *image, const string &file)
//! @brief Loads an image, and tries to figure out it's type.
void
Theme::loadImage(Image *image, const string &file)
{
	if (!image || !file.size())
		return;

	// get image type
	vector<string> tokens;
	if ((Util::splitString(file, tokens, ":", 2)) == 2) {
		image->load(tokens[0]);
		image->setImageType(getImageType(tokens[1]));
	} else {
		image->load(file);
	}
}

//! @fn    bool allocColor(const string &color, XColor *xcolor)
//! @brief Allocs a XColor named color and puts it in xcolor
bool
Theme::allocColor(const string &color, XColor *xcolor)
{
	XColor dummy;

	return (XAllocNamedColor(scr->getDisplay(), scr->getColormap(),
													 color.c_str(), xcolor, &dummy));
}
