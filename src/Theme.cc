//
// Theme.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Theme.hh"

#include "ScreenInfo.hh"
#include "Config.hh"
#include "WindowObject.hh"
#include "Button.hh"
#include "PekwmFont.hh"
#include "Image.hh"
#include "Util.hh"

#include <iostream>

extern "C" {
#include <X11/xpm.h>
}

using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::vector;

const char *DEF_FONT = "fixed";

Theme::UnsignedListItem Theme::_justifylist[] = {
	{"LEFT", LEFT_JUSTIFY},
	{"CENTER", CENTER_JUSTIFY},
	{"RIGHT", RIGHT_JUSTIFY},
	{"", NO_JUSTIFY}
};

Theme::UnsignedListItem Theme::_pixmaptype_list[] = {
	{"TILED", IMAGE_TILED},
	{"SCALED", IMAGE_SCALED},
	{"TRANS", IMAGE_TRANSPARENT},
	{"", NO_IMAGETYPE}
};

Theme::Theme(ScreenInfo *s, Config *c) :
_scr(s), _cfg(c),
_is_loaded(false),
_invert_gc(None),
_win_titleheight(15), _win_titlepadding(1),
_win_fontjustify(CENTER_JUSTIFY),
_menu_padding(2), _menu_fontjustify(LEFT_JUSTIFY),
_menu_borderwidth(1)
{
	// window gc's
	XGCValues gv;
	gv.function = GXinvert;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 1;
	_invert_gc = XCreateGC(_scr->getDisplay(), _scr->getRoot(),
												 GCFunction|GCSubwindowMode|GCLineWidth, &gv);

	// Window
	_win_font = new PekwmFont(_scr);

	// titlebar
	_win_image_fo = new Image(_scr->getDisplay());
	_win_image_fose = new Image(_scr->getDisplay());
	_win_image_un = new Image(_scr->getDisplay());
	_win_image_unse = new Image(_scr->getDisplay());

	// titlebar separator
	_win_sep_fo = new Image(_scr->getDisplay());
	_win_sep_un = new Image(_scr->getDisplay());

	// frame border
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		_win_focused_border[i] = new Image(_scr->getDisplay());
		_win_unfocused_border[i] = new Image(_scr->getDisplay());
	}

	// Harbour
#ifdef HARBOUR
	_harbour_image = new Image(_scr->getDisplay());
#endif // HARBOUR

	// Menu
	_menu_font = new PekwmFont(_scr);

	_scr->grabServer();

	setThemeDir(_cfg->getThemeFile());
	load();

	_scr->ungrabServer(true);
}

Theme::~Theme()
{
	unload(); // should clean things up

	XFreeGC(_scr->getDisplay(), _invert_gc);

	delete _win_font;

	// titlebar
	delete _win_image_fo;
	delete _win_image_fose;
	delete _win_image_un;
	delete _win_image_unse;

	// titlebar separator
	delete _win_sep_fo;
	delete _win_sep_un;

	// frame border
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		delete _win_focused_border[i];
		delete _win_unfocused_border[i];
	}

#ifdef HARBOUR
	delete _harbour_image;
#endif // HARBOUR

	delete _menu_font;
}

//! @fn    void setThemeDir(const string &dir)
//! @bried Sets the path to the theme, make sure's there's an ending /
void
Theme::setThemeDir(const string &dir)
{
	if (!dir.size())
		return;

	_theme_dir = dir;

	if (_theme_dir.at(_theme_dir.size() - 1) != '/')
		_theme_dir.append("/");
}

//! @fn    void load(void)
//! @brief Loads the "ThemeFile", unloads any previous loaded theme.
void
Theme::load(void)
{
	if (_is_loaded)
		unload();

	BaseConfig theme;
	BaseConfig::CfgSection *cs, *sect;

	string theme_file = _theme_dir + string("theme");
	if (!theme.load(theme_file)) {
		theme_file = DATADIR "/themes/default";
		if (!theme.load(theme_file)) {
			cerr << "Couldn't load themedir: " << _theme_dir
					 <<	" or default theme!" << endl;
			setupEmergencyTheme();
			return;
		}
	}

	// temporary variables used in parsing
	string s_value;

	if ((cs = theme.getSection("WINDOW"))) {
		cs->getValue("TITLEHEIGHT", _win_titleheight);
		cs->getValue("TITLEPADDING", _win_titlepadding);

		if ((sect = cs->getSection("FONT"))) {
			sect->getValue("NAME", s_value);
			_win_font->load(s_value);
			sect->getValue("JUSTIFY", s_value);
			_win_fontjustify = getFontJustify(s_value);
			if (_win_fontjustify == NO_JUSTIFY)
				_win_fontjustify = LEFT_JUSTIFY;
		}

		if ((sect = cs->getSection("FOCUSED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &_win_text_fo))
				_win_text_fo.pixel = _scr->getBlackPixel();

			if (sect->getValue("PIXMAP", s_value))
				loadImage(_win_image_fo, _theme_dir + s_value);
			if (sect->getValue("SEPARATOR", s_value))
				loadImage(_win_sep_fo, _theme_dir + s_value);

			loadBorder(sect->getSection("BORDER"), _win_focused_border);
		}

		if ((sect = cs->getSection("UNFOCUSED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &_win_text_un))
				_win_text_un.pixel = _scr->getBlackPixel();

			if (sect->getValue("PIXMAP", s_value))
				loadImage(_win_image_un, _theme_dir + s_value);
			if (sect->getValue("SEPARATOR", s_value))
				loadImage(_win_sep_un, _theme_dir + s_value);

			loadBorder(sect->getSection("BORDER"), _win_unfocused_border);
		}

		if ((sect = cs->getSection("SELECTED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &_win_text_fose))
				_win_text_fose.pixel = _scr->getWhitePixel();
			if (sect->getValue("PIXMAP", s_value))
				loadImage(_win_image_fose, _theme_dir + s_value);
		}

		if ((sect = cs->getSection("UNFOCUSEDSELECTED"))) {
			sect->getValue("TEXT", s_value);
			if (!allocColor(s_value, &_win_text_unse))
				_win_text_unse.pixel = _scr->getBlackPixel();
			if (sect->getValue("PIXMAP", s_value))
				loadImage(_win_image_unse, _theme_dir + s_value);
		}

		if ((sect = cs->getSection("BUTTONS")))
			loadButtons(sect);
	}

	if ((cs = theme.getSection("MENU"))) {
		cs->getValue("FONT", s_value);
		_menu_font->load(s_value);
		cs->getValue("PADDING", _menu_padding);
		cs->getValue("TEXTJUSTIFY", s_value);
		_menu_fontjustify = getFontJustify(s_value);
		if (_menu_fontjustify == NO_JUSTIFY)
			_menu_fontjustify = LEFT_JUSTIFY;

		// Load colors
		cs->getValue("TEXTCOLOR", s_value);
		if (!allocColor(s_value, &_menu_text))
			_menu_text.pixel = _scr->getWhitePixel();
		cs->getValue("TEXTCOLORSELECTED", s_value);
		if (!allocColor(s_value, &_menu_text_se))
			_menu_text_se.pixel = _scr->getBlackPixel();
		cs->getValue("TEXTCOLORTITLE", s_value);
		if (!allocColor(s_value, &_menu_text_ti))
			_menu_text_ti.pixel = _scr->getWhitePixel();
		cs->getValue("BACKGROUND", s_value);
		if (!allocColor(s_value, &_menu_background))
			_menu_background.pixel = _scr->getBlackPixel();
		cs->getValue("BACKGROUNDSELECTED", s_value);
		if (!allocColor(s_value, &_menu_background_se))
			_menu_background_se.pixel = _scr->getWhitePixel();
		cs->getValue("BACKGROUNDTITLE", s_value);
		if (!allocColor(s_value, &_menu_background_ti))
			_menu_background_ti.pixel = _scr->getWhitePixel();
		cs->getValue("BORDERCOLOR", s_value);
		if (!allocColor(s_value, &_menu_border))
			_menu_border.pixel = _scr->getBlackPixel();
		cs->getValue("BORDERWIDTH", _menu_borderwidth);

		// Set the color of the font
		_menu_font->setColor(_menu_text);
	}

	if ((cs = theme.getSection("ROOT"))) {
		if (cs->getValue("COMMAND", s_value))
			Util::forkExec(s_value);
	}


#ifdef HARBOUR
	if ((cs = theme.getSection("HARBOUR"))) {
		if (cs->getValue("PIXMAP", s_value))
			loadImage(_harbour_image, _theme_dir + s_value);
	}
#endif // HARBOUR

	_is_loaded = true;
}

//! @fn    void setupEmergencyTheme(void)
//! @brief Sets a theme up incase no theme can be found.
void
Theme::setupEmergencyTheme(void)
{
	if (_is_loaded)
		unload(); // unload last theme

	// Window
	_win_titleheight = 15;
	_win_titlepadding = 1;
	_win_font->load(DEF_FONT);
	_win_fontjustify = CENTER_JUSTIFY;

	// titlebar text
	_win_text_fo.pixel = _scr->getBlackPixel();
	_win_text_fose.pixel = _scr->getWhitePixel();
	_win_text_un.pixel = _scr->getBlackPixel();
	_win_text_unse.pixel = _scr->getWhitePixel();

	// setup menu colors
	_menu_font->load(DEF_FONT);
	_menu_padding = 2;
	_menu_fontjustify = LEFT_JUSTIFY;

	_menu_text.pixel = _scr->getBlackPixel();
	_menu_text_se.pixel = _scr->getWhitePixel();
	_menu_text_ti.pixel = _scr->getWhitePixel();
	_menu_background.pixel = _scr->getWhitePixel();
	_menu_background_se.pixel = _scr->getBlackPixel();
	_menu_background_ti.pixel = _scr->getBlackPixel();
	_menu_border.pixel = _scr->getBlackPixel();

	_is_loaded = true;
}

//! @fn    void unload(void)
//! @brief Unloads all pixmaps, fonts, gc and colors allocated by the theme.
void
Theme::unload(void) {
	// unload title buttons
	list<ButtonData*>::iterator b_it = _button_list.begin();
	for (; b_it != _button_list.end(); ++b_it) {
		for (unsigned int i = 0; i < BUTTON_NO_STATE; ++i) {
			XFreePixmap(_scr->getDisplay(), (*b_it)->pixmap[i]);
			XFreePixmap(_scr->getDisplay(), (*b_it)->shape[i]);
		}
		delete (*b_it);
	}
	_button_list.clear();

	_win_font->unload();

	// unload titlebar
	_win_image_fo->unload();
	_win_image_fose->unload();
	_win_image_un->unload();
	_win_image_unse->unload();

	// unload titlebar separators
	_win_sep_fo->unload();
	_win_sep_un->unload();

	// unload border pixmaps
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		_win_focused_border[i]->unload();
		_win_unfocused_border[i]->unload();
	}

	// free window pixels
	unsigned long win_pixels[] =
		{_win_text_fo.pixel, _win_text_un.pixel,
		 _win_text_fose.pixel, _win_text_unse.pixel};
	XFreeColors(_scr->getDisplay(), _scr->getColormap(), win_pixels, 4, 0);

#ifdef HARBOUR
	_harbour_image->unload();
#endif // HARBOUR

	// unload menu
	_menu_font->unload();
	unsigned long menu_pixels[] =
		 {_menu_text.pixel, _menu_text_se.pixel, _menu_text_ti.pixel,
			_menu_background.pixel, _menu_background_se.pixel,
			_menu_background_ti.pixel, _menu_border.pixel};
	XFreeColors(_scr->getDisplay(), _scr->getColormap(), menu_pixels, 7, 0);

	_is_loaded = false;
}

//! @fn    TextJustify getFontJustify(const string &justify)
//! @brief
TextJustify
Theme::getFontJustify(const string &justify)
{
	if (!justify.size())
		return NO_JUSTIFY;

	for (unsigned int i = 0; _justifylist[i].value != NO_JUSTIFY; ++i) {
		if (_justifylist[i] == justify) {
			return TextJustify(_justifylist[i].value);
		}
	}

	return NO_JUSTIFY;
}

//! @fn    Image::ImageType getImageType(const string& type)
//! @brief
ImageType
Theme::getImageType(const string& type)
{
	if (!type.size())
		return IMAGE_TILED;

	for (unsigned int i = 0; _pixmaptype_list[i].value != NO_IMAGETYPE; ++i) {
		if (_pixmaptype_list[i] == type)
			return ImageType(_pixmaptype_list[i].value);
	}

	return IMAGE_TILED;
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
			data[BORDER_TOP_LEFT]->load(_theme_dir + tokens[0]);
			data[BORDER_TOP]->load(_theme_dir + tokens[1]);
			data[BORDER_TOP_RIGHT]->load(_theme_dir + tokens[2]);
		}
		tokens.clear();
	}

	if (cs->getValue("SIDE", s_value)) {
		if ((Util::splitString(s_value, tokens, " \t", 2)) == 2) {
			data[BORDER_LEFT]->load(_theme_dir + tokens[0]);
			data[BORDER_RIGHT]->load(_theme_dir + tokens[1]);
		}
		tokens.clear();
	}

	if (cs->getValue("BOTTOM", s_value)) {
		if ((Util::splitString(s_value, tokens, " \t", 3)) == 3) {
			data[BORDER_BOTTOM_LEFT]->load(_theme_dir + tokens[0]);
			data[BORDER_BOTTOM]->load(_theme_dir + tokens[1]);
			data[BORDER_BOTTOM_RIGHT]->load(_theme_dir + tokens[2]);
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

	BaseConfig::CfgSection *sect, *sub;

	// temporary parsing data
	ActionEvent ae;
	bool left;
	unsigned int w, h;

	string t_actions, t_pixmaps;
	vector<string> tokens;
	vector<string>::iterator it;

	while ((sect = cs->getNextSection())) {
		if (*sect == "LEFT") {
			left = true;
		} else if (*sect == "RIGHT") {
			left = false;
		} else
			continue; // not a button section

		if (sect->getValue("PIXMAPS", t_pixmaps)) {
			tokens.clear();
			if (Util::splitString(t_pixmaps, tokens, " \t", 3) == 3) {
				ButtonData *button = new ButtonData();

				it = tokens.begin();
				for (unsigned int i = 0; it != tokens.end(); ++i, ++it)
					loadXpm(_theme_dir + *it, button->pixmap[i], button->shape[i], w, h);

				button->width = w;
				button->height = h;
				button->left = left;

				while ((sub = sect->getNextSection())) {
					if (_cfg->parseActionEvent(sub, ae, BUTTONCLICK_OK, true))
							button->ae_list.push_back(ae);
				}

				_button_list.push_back(button);
			}
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
	attr.visual = _scr->getVisual();

	if ((XpmReadFileToPixmap(_scr->getDisplay(), _scr->getRoot(),
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

	return (XAllocNamedColor(_scr->getDisplay(), _scr->getColormap(),
													 color.c_str(), xcolor, &dummy));
}
