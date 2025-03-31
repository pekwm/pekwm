//
// Theme.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2003-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "Exception.hh"
#include "X11.hh"
#include "Theme.hh"
#include "ThemeUtil.hh"
#include "Util.hh"
#include "String.hh"

#include "Color.hh"
#include "ColorPalette.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "PFont.hh"
#include "PTexture.hh"
#include "TextureHandler.hh"
#include "TkGlobals.hh"

#include <iostream>
#include <string>

extern "C" {
#include <assert.h>
#include <stdlib.h>
}

#define DEFAULT_FONT "Sans-12"
#define DEFAULT_LARGE_FONT "Sans-14:weight=bold"
#define DEFAULT_HEIGHT 17U
#define DEFAULT_HEIGHT_STR "17"

static void
parsePad(float scale, const std::string& str, int *pad)
{
	std::vector<std::string> tok;
	if (Util::splitString(str, tok, " \t", 4) == 4) {
		for (uint i = 0; i < PAD_NO; ++i) {
			pad[i] = ThemeUtil::parsePixel(scale, tok[i], 0);
		}
	}
}

/**
 * Calculate pad_up and pad_down to make Font vertically centered on the
 * available height.
 */
static void
calculatePadAdapt(const int height_available, PFont* font, int& pad_up,
		  int& pad_down)
{
	float height = font->getHeight();
	if (height_available < static_cast<int>(font->getHeight())) {
		// Keep user settings if we can't adjust height
		return;
	}

	float pad = height_available - height;
	if (font->useAscentDescent()) {
		pad_up = static_cast<int>(pad / (height / font->getAscent()));
		pad_down = static_cast<int>(pad / (height / font->getDescent()));
	} else {
		pad_up = static_cast<int>(pad / 2);
		pad_down = static_cast<int>(pad / 2);
	}
}

// Theme::ColorMap
bool
Theme::ColorMap::load(CfgParser::Entry *section, std::map<int, int> &map)
{
	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		if (! pekwm::ascii_ncase_equal((*it)->getName(), "MAP")) {
			USER_WARN("unexpected entry " << (*it)->getName()
				  << " in ColorMap");
			continue;
		}

		CfgParser::Entry *to = (*it)->getSection()
			? (*it)->getSection()->findEntry("TO") : nullptr;
		if (to == nullptr) {
			USER_WARN("missing To entry in ColorMap entry");
		} else {
			int from_c, to_c;
			try {
				from_c = parseColor((*it)->getValue());
			} catch (ValueException& ex) {
				std::ostringstream msg;
				msg << "invalid from color "
				    << ex.getValue()
				    << " in ColorMap entry";
				USER_WARN(msg.str());
				continue;
			}
			try {
				to_c = parseColor(to->getValue());
			} catch (ValueException& ex) {
				std::ostringstream msg;
				msg << "invalid to color "
				    << ex.getValue()
				    << " in ColorMap entry";
				USER_WARN(msg.str());
				continue;
			}

			map[from_c] = to_c;
		}
	}
	return true;
}

int
Theme::ColorMap::parseColor(const std::string& desc)
{
	std::string str = pekwm::getColorResource(desc);
	if (str.size() != 7 || str[0] != '#') {
		throw ValueException(str);
	}

	// ARGB format, always set full alpha
	const char *p = str.c_str() + 1;
	uchar cbuf[4] = {255, parseHex(p), parseHex(p + 2), parseHex(p + 4)};
	int c;
	memcpy(&c, cbuf, sizeof(cbuf));
	return c;
}

uchar
Theme::ColorMap::parseHex(const char *p)
{
	char val[3] = {p[0], p[1], 0};
	return static_cast<uchar>(strtol(val, NULL, 16));
}

// Theme::PDecorButtonData

Theme::PDecorButtonData::PDecorButtonData(TextureHandler *th)
	: _th(th),
	  _loaded(false),
	  _shape(true),
	  _left(false),
	  _width(1),
	  _height(1)
{
	memset(_texture, 0, sizeof(_texture));
}

Theme::PDecorButtonData::~PDecorButtonData(void)
{
	unload();
}

//! @brief Parses CfgParser::Entry section, loads and verifies data.
//! @param section CfgParser::Entry with button configuration.
//! @return True if a valid button was parsed.
bool
Theme::PDecorButtonData::load(CfgParser::Entry *section)
{
	if (*section == "LEFT") {
		_left = true;
	} else if (*section == "RIGHT") {
		_left = false;
	} else {
		check();
		return false;
	}
	_loaded = true;

	// Get actions.
	ActionEvent ae;
	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		if (ActionConfig::parseActionEvent(*it, ae, BUTTONCLICK_OK,
						   true)) {
			_aes.push_back (ae);
		}
	}

	// Got some actions, consider it to be a valid button.
	if (_aes.size() > 0) {
		CfgParser::Entry *value;

		value = section->findEntry("SETSHAPE");
		if (value) {
			_shape = Util::isTrue(value->getValue());
		}

		value = section->findEntry("FOCUSED");
		if (value) {
			_texture[BUTTON_STATE_FOCUSED] =
				_th->getTexture(value->getValue());
		}
		value = section->findEntry("UNFOCUSED");
		if (value) {
			_texture[BUTTON_STATE_UNFOCUSED] =
				_th->getTexture(value->getValue());
		}
		value = section->findEntry("PRESSED");
		if (value) {
			_texture[BUTTON_STATE_PRESSED] =
				_th->getTexture(value->getValue());
		}

		// HOOVER has been kept around due to backwards compatibility.
		value = section->findEntry("HOVER");
		if (! value) {
			value = section->findEntry("HOOVER");
		}
		if (value) {
			_texture[BUTTON_STATE_HOVER] =
				_th->getTexture(value->getValue());
		}

		check();

		return true;
	}

	return false;
}

//! @brief Unloads data.
void
Theme::PDecorButtonData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
		_th->returnTexture(&_texture[i]);
	}
}

//! @brief Verifies and makes sure no 0 textures exists.
void
Theme::PDecorButtonData::check(void)
{
	for (uint i = 0; i < (BUTTON_STATE_NO - 1); ++i) {
		if (! _texture[i]) {
			_texture[i] = _th->getTexture("Solid #999999");
		}
	}

	_width = _texture[BUTTON_STATE_FOCUSED]->getWidth();
	_height = _texture[BUTTON_STATE_FOCUSED]->getHeight();

	_loaded = true;
}

// Theme::PDecorData


static const char *focused_state_to_string[FOCUSED_STATE_NO] =
	{"FOCUSED", "UNFOCUSED", "FOCUSEDSELECTED", "UNFOCUSEDSELECTED"};
static const char *border_to_string[BORDER_NO_POS] =
	{"TOPLEFT", "TOPRIGHT",
	 "BOTTOMLEFT", "BOTTOMRIGHT",
	 "TOP", "LEFT", "RIGHT", "BOTTOM"};

Theme::PDecorData::PDecorData(FontHandler* fh, TextureHandler* th,
			      int version, const char *name)
	: _fh(fh),
	  _th(th),
	  _version(version),
	  _loaded(false),
	  _title_height(ThemeUtil::scaledPixelValue(th->getScale(),
						    DEFAULT_HEIGHT)),
	  _title_width_min(0),
	  _title_width_max(100),
	  _title_width_symetric(true),
	  _title_height_adapt(false),
	  _title_align_to_side(false)
{
	if (name) {
		_name = name;
	}

	// init arrays
	memset(_pad, 0, sizeof(_pad));
	memset(_texture_tab, 0, sizeof(_texture_tab));
	memset(_font, 0, sizeof(_font));
	memset(_font_color, 0, sizeof(_font_color));
	memset(_texture_main, 0, sizeof(_texture_main));
	memset(_texture_separator, 0, sizeof(_texture_separator));
	memset(_texture_border, 0, sizeof(_texture_border));
}

Theme::PDecorData::~PDecorData(void)
{
	unload();
}

int
Theme::PDecorData::getPad(PadType pad) const
{
	return _pad[(pad != PAD_NO) ? pad : 0];
}

/**
 * Parses CfgParser::Entry section, loads and verifies data.
 *
 * @param section CfgParser::Entry with pdecor configuration.
 * @return true if a valid decor was constructed, else false.
 */
bool
Theme::PDecorData::load(CfgParser::Entry *section)
{
	if (! section) {
		check();
		return false;
	}

	_name = section->getValue();
	if (! _name.size()) {
		USER_WARN("no name identifying decor");
		return false;
	}

	CfgParser::Entry *title_section = section->findSection("TITLE");
	if (! title_section) {
		// no longer require a Title section, everything in the decor
		// is inside of it anyway.
		title_section = section;
	}
	_loaded = true;

	ThemeUtil::CfgParserKeys keys(_th->getScale());
	bool title_pad_adapt = _version > 2;
	std::string value_pad, value_focused, value_unfocused;

	keys.add_pixels("HEIGHT", _title_height, DEFAULT_HEIGHT);
	keys.add_numeric<uint>("WIDTHMIN", _title_width_min, 0);
	keys.add_numeric<uint>("WIDTHMAX", _title_width_max, 100, 0, 100);
	keys.add_bool("WIDTHSYMETRIC", _title_width_symetric, true);
	keys.add_bool("HEIGHTADAPT", _title_height_adapt, false);
	keys.add_bool("TITLEALIGNTOSIDE", _title_align_to_side, false);
	keys.add_string("PAD", value_pad, "0 0 0 0", 7);
	keys.add_bool("PADADAPT", title_pad_adapt, title_pad_adapt);
	keys.add_string("FOCUSED", value_focused, "Empty",
			_th->getLengthMin());
	keys.add_string("UNFOCUSED", value_unfocused, "Empty",
			_th->getLengthMin());

	title_section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	// Handle parsed data.
	_texture_main[FOCUSED_STATE_FOCUSED] =
		_th->getTexture(value_focused);
	_texture_main[FOCUSED_STATE_UNFOCUSED] =
		_th->getTexture(value_unfocused);
	parsePad(_th->getScale(), value_pad, _pad);

	CfgParser::Entry *tab_section = title_section->findSection("TAB");
	if (tab_section) {
		for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
			const char *fs_str = focused_state_to_string[i];
			CfgParser::Entry *value =
				tab_section->findEntry(fs_str);
			if (value) {
				_texture_tab[i] =
					_th->getTexture(value->getValue());
			}
		}
	}

	CfgParser::Entry *separator_section =
		title_section->findSection("SEPARATOR");
	if (separator_section) {
		keys.add_string("FOCUSED", value_focused, "Empty",
				_th->getLengthMin());
		keys.add_string("UNFOCUSED", value_unfocused, "Empty",
				_th->getLengthMin());

		separator_section->parseKeyValues(keys.begin(), keys.end());
		keys.clear();

		// Handle parsed data.
		_texture_separator[FOCUSED_STATE_FOCUSED] =
			_th->getTexture(value_focused);
		_texture_separator[FOCUSED_STATE_UNFOCUSED] =
			_th->getTexture(value_unfocused);
	}


	CfgParser::Entry *font_section = title_section->findSection("FONT");
	if (font_section) {
		for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
			const char* fs_str = focused_state_to_string[i];
			CfgParser::Entry *value =
				font_section->findEntry(fs_str);
			if (value) {
				_font[i] = _fh->getFont(value->getValue());
			}
		}
	} else {
		P_WARN("no font section in decor: " << _name);
	}

	CfgParser::Entry *fontcolor_section =
		title_section->findSection("FONTCOLOR");
	if (fontcolor_section) {
		for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
			const char* fs_str = focused_state_to_string[i];
			CfgParser::Entry *value =
				fontcolor_section->findEntry(fs_str);
			if (value) {
				_font_color[i] =
					_fh->getColor(value->getValue());
			}
		}
	}

	loadButtons(title_section->findSection("BUTTONS"));
	loadBorder(title_section->findSection("BORDER"));

	check();

	if (title_pad_adapt && ! _title_height_adapt) {
		// this is the only required state and the one that always
		// have a font available so use it for calculations
		PFont* font = _font[FOCUSED_STATE_FOCUSED];
		calculatePadAdapt(_title_height, font, _pad[PAD_UP],
				  _pad[PAD_DOWN]);
	}

	return true;
}

void
Theme::PDecorData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
		_th->returnTexture(&_texture_tab[i]);
		_fh->returnFont(&_font[i]);
		_fh->returnColor(&_font_color[i]);
	}

	for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
		_th->returnTexture(&_texture_main[i]);
		_th->returnTexture(&_texture_separator[i]);

		for (uint j = 0; j < BORDER_NO_POS; ++j) {
			_th->returnTexture(&_texture_border[i][j]);
		}
	}

	std::vector<Theme::PDecorButtonData*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		delete *it;
	}
	_buttons.clear();
}

void
Theme::PDecorData::check()
{
	// check values
	if (_title_width_max > 100) {
		P_WARN(_name << " WIDTHMAX > 100");
		_title_width_max = 100;
	}

	checkTextures();
	checkFonts();
	checkBorder();
	checkColors();

	_loaded = true;
}

//! @brief Loads border data.
void
Theme::PDecorData::loadBorder(CfgParser::Entry *section)
{
	if (! section) {
		return;
	}

	CfgParser::Entry *sub, *value;

	sub = section->findSection("FOCUSED");
	if (sub) {
		for (uint i = 0; i < BORDER_NO_POS; ++i) {
			value = sub->findEntry(border_to_string[i]);
			if (value) {
				_texture_border[FOCUSED_STATE_FOCUSED][i] =
					_th->getTexture(value->getValue());
			}
		}
	}

	sub = section->findSection("UNFOCUSED");
	if (sub) {
		for (uint i = 0; i < BORDER_NO_POS; ++i) {
			value = sub->findEntry(border_to_string[i]);
			if (value) {
				_texture_border[FOCUSED_STATE_UNFOCUSED][i] =
					_th->getTexture(value->getValue());
			}
		}
	}
}

//! @brief Loads button data.
void
Theme::PDecorData::loadButtons(CfgParser::Entry *section)
{
	if (! section) {
		return;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		if (! (*it)->getSection()) {
			continue;
		}

		Theme::PDecorButtonData *btn = new Theme::PDecorButtonData(_th);
		if (btn->load((*it)->getSection())) {
			_buttons.push_back(btn);
		} else {
			delete btn;
		}
	}
}

//! @brief Checks for 0 textures, prints warning and sets empty texture
void
Theme::PDecorData::checkTextures(void)
{
	const char *textures[] = {
		"Solid #eeeeee",
		"Solid #cccccc",
		"Solid #ffffff",
		"Solid #dddddd"
	};

	for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
		if (! _texture_tab[i]) {
			P_WARN(_name << " missing tab texture state "
			       << focused_state_to_string[i]);
			_texture_tab[i] = _th->getTexture(textures[i]);
		}
	}
	for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
		if (! _texture_main[i]) {
			P_WARN(_name << " missing main texture state "
			       << focused_state_to_string[i]);
			_texture_main[i] = _th->getTexture(textures[i]);
		}
		if (! _texture_separator[i]) {
			P_WARN(_name << " missing tab texture state "
			       << focused_state_to_string[i]);
			_texture_separator[i] =
				_th->getTexture("Solid #999999 2x"
						DEFAULT_HEIGHT_STR);
		}
	}
}

//! @brief Checks for 0 fonts, prints warning and sets empty font
void
Theme::PDecorData::checkFonts(void)
{
	// the only font that's "obligatory" is the standard focused font,
	// others are only used if availible so we only check the focused font.
	if (! _font[FOCUSED_STATE_FOCUSED]) {
		P_WARN(_name << " missing font state "
		       << focused_state_to_string[FOCUSED_STATE_FOCUSED]);
		_font[FOCUSED_STATE_FOCUSED] = _fh->getFont(DEFAULT_FONT);
	}
}

//! @brief Checks for 0 border PTextures.
void
Theme::PDecorData::checkBorder(void)
{
	for (uint state = FOCUSED_STATE_FOCUSED;
	     state < FOCUSED_STATE_FOCUSED_SELECTED; ++state) {
		for (uint i = 0; i < BORDER_NO_POS; ++i) {
			if (! _texture_border[state][i]) {
				if (Debug::isLevel(Debug::LEVEL_WARN)) {
					std::ostringstream msg;
					msg << _name
					    << " missing border texture ";
					msg << border_to_string[i] << " ";
					msg << focused_state_to_string[state];
					P_WARN(msg.str());
				}
				_texture_border[state][i] =
					_th->getTexture("Solid #999999 2x2");
			}
		}
	}
}

//! @brief Checks for 0 colors, prints warning and sets empty color
void
Theme::PDecorData::checkColors(void)
{
	for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
		if (! _font_color[i]) {
			P_WARN(_name << " missing font color state "
			       << focused_state_to_string[i]);
			_font_color[i] = _fh->getColor("#000000");
		}
	}
}

// Theme::PMenuData

Theme::PMenuData::PMenuData(FontHandler* fh, TextureHandler* th,
			    int version)
	: _fh(fh),
	  _th(th),
	  _version(version),
	  _loaded(false)
{
	memset(_font, 0, sizeof(_font));
	memset(_color, 0, sizeof(_color));
	memset(_tex_menu, 0, sizeof(_tex_menu));
	memset(_tex_item, 0, sizeof(_tex_item));
	memset(_tex_arrow, 0, sizeof(_tex_arrow));
	memset(_tex_sep, 0, sizeof(_tex_sep));
	memset(_pad, 0, sizeof(_pad));
}

Theme::PMenuData::~PMenuData(void)
{
	unload();
}

//! @brief Parses CfgParser::Entry section, loads and verifies data.
//! @param section CfgParser::Entry with pmenu configuration.
bool
Theme::PMenuData::load(CfgParser::Entry *section)
{
	if (! section) {
		check();
		return false;
	}
	_loaded = true;

	CfgParserKeys keys;
	std::string value_pad;
	bool pad_adapt = _version > 2;
	keys.add_string("PAD", value_pad, "0 0 0 0", 7);
	keys.add_bool("PADADAPT", pad_adapt, pad_adapt);

	parsePad(_th->getScale(), value_pad, _pad);

	const char *states[] = {"FOCUSED", "UNFOCUSED", "SELECTED", nullptr};
	for (int i = 0; states[i] != nullptr; i++) {
		CfgParser::Entry *value = section->findSection(states[i]);
		if (value) {
			loadState(value, static_cast<ObjectState>(i));
		}
	}

	check();

	if (pad_adapt) {
		PFont* font = _font[OBJECT_STATE_FOCUSED];
		calculatePadAdapt(
			font->getHeight() + _pad[PAD_UP] + _pad[PAD_DOWN],
			font, _pad[PAD_UP], _pad[PAD_DOWN]);
	}

	return true;
}

void
Theme::PMenuData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
		_fh->returnFont(&_font[i]);
		_fh->returnColor(&_color[i]);

		_th->returnTexture(&_tex_menu[i]);
		_th->returnTexture(&_tex_item[i]);
		_th->returnTexture(&_tex_arrow[i]);
	}

	for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
		_th->returnTexture(&_tex_sep[i]);
	}
}

void
Theme::PMenuData::check()
{
	for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
		if (! _font[i]) {
			_font[i] = _fh->getFont(DEFAULT_FONT);
		}
		if (! _color[i]) {
			_color[i] = _fh->getColor("#000000");
		}
		if (! _tex_menu[i]) {
			_tex_menu[i] = _th->getTexture("Solid #ffffff");
		}
		if (! _tex_item[i]) {
			_tex_item[i] = _th->getTexture("Solid #ffffff");
		}
		if (! _tex_arrow[i]) {
			_tex_arrow[i] = _th->getTexture("Solid #000000 2x2");
		}
	}

	for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
		if (! _tex_sep[i]) {
			_tex_sep[i] = _th->getTexture("Solid #000000 1x1");
		}
	}

	_loaded = true;
}

void
Theme::PMenuData::loadState(CfgParser::Entry *section, ObjectState state)
{
	CfgParserKeys keys;
	std::string value_font, value_background, value_item;
	std::string value_text, value_arrow, value_separator;

	keys.add_string("FONT", value_font);
	keys.add_string("BACKGROUND", value_background, "Solid #ffffff");
	keys.add_string("ITEM", value_item, "Solid #ffffff");
	keys.add_string("TEXT", value_text, "#000000");
	keys.add_string("ARROW", value_arrow, "Solid #000000 2x2");
	if (state < OBJECT_STATE_SELECTED) {
		keys.add_string("SEPARATOR", value_separator,
				"Solid #000000 1x1");
	}

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	// Handle parsed data.
	_font[state] = _fh->getFont(value_font);
	_tex_menu[state] = _th->getTexture(value_background);
	_tex_item[state] = _th->getTexture(value_item);
	_color[state] = _fh->getColor(value_text);
	_tex_arrow[state] = _th->getTexture(value_arrow);
	if (state < OBJECT_STATE_SELECTED) {
		_tex_sep[state] = _th->getTexture(value_separator);
	}
}

// Theme::TextDialogData

Theme::TextDialogData::TextDialogData(FontHandler* fh, TextureHandler* th,
				      int version)
	: _fh(fh),
	  _th(th),
	  _version(version),
	  _loaded(false),
	  _font(0),
	  _color(0),
	  _tex(0)
{
	memset(_pad, 0, sizeof(_pad));
}

Theme::TextDialogData::~TextDialogData(void)
{
	unload();
}

//! @brief Parses CfgParser::Entry section, loads and verifies data.
//! @param section CfgParser::Entry with textdialog configuration.
bool
Theme::TextDialogData::load(CfgParser::Entry *section)
{
	if (! section) {
		check();
		return false;
	}
	_loaded = true;

	CfgParserKeys keys;
	std::string value_font, value_text, value_texture, value_pad;
	bool pad_adapt = _version > 2;

	keys.add_string("FONT", value_font);
	keys.add_string("TEXT", value_text, "#000000");
	keys.add_string("TEXTURE", value_texture, "Solid #ffffff");
	keys.add_string("PAD", value_pad, "0 0 0 0", 7);
	keys.add_bool("PADADAPT", pad_adapt, pad_adapt);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	// Handle parsed data.
	_font = _fh->getFont(value_font);
	_color = _fh->getColor(value_text);
	_tex = _th->getTexture(value_texture);

	parsePad(_th->getScale(), value_pad, _pad);

	check();

	if (pad_adapt) {
		calculatePadAdapt(
			_font->getHeight() + _pad[PAD_UP] + _pad[PAD_DOWN],
			_font, _pad[PAD_UP], _pad[PAD_DOWN]);
	}

	return true;
}

void
Theme::TextDialogData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	_fh->returnFont(&_font);
	_fh->returnColor(&_color);
	_th->returnTexture(&_tex);
}

//! @brief Check data properties, prints warning and tries to fix.
//! @todo print warnings
void
Theme::TextDialogData::check(void)
{
	if (! _font) {
		_font = _fh->getFont(DEFAULT_FONT);
	}
	if (! _color) {
		_color = _fh->getColor("#000000");
	}
	if (! _tex) {
		_tex = _th->getTexture("Solid #ffffff");
	}

	_loaded = true;
}

// WorkspaceIndicatorData

/**
 * WorkspaceIndicatorData constructor
 */
Theme::WorkspaceIndicatorData::WorkspaceIndicatorData(FontHandler* fh,
						      TextureHandler *th)
	: _fh(fh),
	  _th(th),
	  _loaded(false),
	  font(nullptr),
	  font_color(nullptr),
	  texture_background(nullptr),
	  texture_workspace(nullptr),
	  texture_workspace_act(nullptr),
	  edge_padding(0),
	  workspace_padding(0)
{
}

/**
 * WorkspaceIndicatorData destructor
 */
Theme::WorkspaceIndicatorData::~WorkspaceIndicatorData(void)
{
	unload();
}

/**
 * Load theme data and check.
 */
bool
Theme::WorkspaceIndicatorData::load(CfgParser::Entry *section)
{
	if (! section) {
		check();
		return false;
	}
	_loaded = true;

	ThemeUtil::CfgParserKeys keys(_th->getScale());

	std::string value_font, value_color, value_tex_bg;
	std::string value_tex_ws, value_tex_ws_act;

	keys.add_string("FONT", value_font);
	keys.add_string("TEXT", value_color);
	keys.add_string("BACKGROUND", value_tex_bg);
	keys.add_string("WORKSPACE", value_tex_ws);
	keys.add_string("WORKSPACEACTIVE", value_tex_ws_act);
	keys.add_pixels("EDGEPADDING", edge_padding, 5);
	keys.add_pixels("WORKSPACEPADDING", workspace_padding, 2);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	font = _fh->getFont(value_font);
	font_color = _fh->getColor(value_color);
	texture_background = _th->getTexture(value_tex_bg);
	texture_workspace = _th->getTexture(value_tex_ws);
	texture_workspace_act = _th->getTexture(value_tex_ws_act);

	check();

	return true;
}

/**
 * Unload loaded theme data.
 */
void
Theme::WorkspaceIndicatorData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	_fh->returnFont(&font);
	_fh->returnColor(&font_color);
	_th->returnTexture(&texture_background);
	_th->returnTexture(&texture_workspace);
	_th->returnTexture(&texture_workspace_act);

	edge_padding = 0;
	workspace_padding = 0;
}

/**
 * Validate theme data loading
 */
void
Theme::WorkspaceIndicatorData::check(void)
{
	if (! font) {
		font = _fh->getFont(DEFAULT_FONT);
	}
	if (! font_color) {
		font_color = _fh->getColor("#000000");
	}
	if (! texture_background) {
		texture_background = _th->getTexture("Solid #ffffff");
	}
	if (! texture_workspace) {
		texture_workspace = _th->getTexture("Solid #cccccc");
	}
	if (! texture_workspace_act) {
		texture_workspace_act = _th->getTexture("Solid #aaaaaa");
	}

	_loaded = true;
}

Theme::DialogData::DialogData(FontHandler* fh, TextureHandler* th)
	: _fh(fh),
	  _th(th),
	  _loaded(false),
	  _background(nullptr),
	  _button_font(nullptr),
	  _button_color(nullptr),
	  _title_font(nullptr),
	  _title_color(nullptr),
	  _text_font(nullptr),
	  _text_color(nullptr)
{
	memset(_button, 0, sizeof(_button));
	clear();
}

Theme::DialogData::~DialogData(void)
{
	unload();
}

bool
Theme::DialogData::load(CfgParser::Entry *section)
{
	if (section == nullptr) {
		check();
		return false;
	}
	_loaded = true;

	std::string val_bg, val_font, val_text, val_tfont, val_ttext, val_pad;
	CfgParserKeys keys;
	keys.add_string("BACKGROUND", val_bg, "Solid #ffffff");
	keys.add_string("FONT", val_font, DEFAULT_FONT);
	keys.add_string("TEXT", val_text, "#000000");
	keys.add_string("TITLEFONT", val_tfont, DEFAULT_LARGE_FONT);
	keys.add_string("TITLECOLOR", val_ttext, "#000000");
	keys.add_string("PAD", val_pad, "0 0 0 0", 7);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	// Handle parsed data.
	_background = _th->getTexture(val_bg);
	_text_font = _fh->getFont(val_font);
	_text_color = _fh->getColor(val_text);
	_title_font = _fh->getFont(val_tfont);
	_title_color = _fh->getColor(val_ttext);
	parsePad(_th->getScale(), val_pad, _pad);

	CfgParser::Entry *button = section->findSection("BUTTON");
	if (button != nullptr) {
		std::string val_fo, val_un, val_pr, val_ho;
		CfgParserKeys bkeys;
		bkeys.add_string("FONT", val_font, DEFAULT_FONT);
		bkeys.add_string("TEXT", val_text, "#000000");
		bkeys.add_string("FOCUSED", val_fo);
		bkeys.add_string("UNFOCUSED", val_un);
		bkeys.add_string("PRESSED", val_pr);
		bkeys.add_string("HOOVER", val_ho);
		button->parseKeyValues(bkeys.begin(), bkeys.end());
		bkeys.clear();

		_button_font = _fh->getFont(val_font);
		_button_color = _fh->getColor(val_text);
		_button[BUTTON_STATE_FOCUSED] = _th->getTexture(val_fo);
		_button[BUTTON_STATE_UNFOCUSED] = _th->getTexture(val_un);
		_button[BUTTON_STATE_PRESSED] = _th->getTexture(val_pr);
		_button[BUTTON_STATE_HOVER] = _th->getTexture(val_ho);
	}

	check();

	return true;
}

void
Theme::DialogData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	_fh->returnColor(&_text_color);
	_fh->returnFont(&_text_font);
	_fh->returnColor(&_button_color);
	_fh->returnFont(&_title_font);
	for (int i = 0; i < BUTTON_STATE_NO; i++) {
		_th->returnTexture(&_button[i]);
	}
	_fh->returnColor(&_button_color);
	_fh->returnFont(&_button_font);
	_th->returnTexture(&_background);

	clear();
}

void
Theme::DialogData::check(void)
{
	if (_background == nullptr) {
		_background = _th->getTexture("Solid #ffffff");
	}
	if (_button_font == nullptr) {
		_button_font = _fh->getFont(DEFAULT_FONT);
	}
	if (_button_color == nullptr) {
		_button_color = _fh->getColor("#000000");
	}
	const char* button_textures[] = {
		"SolidRaised #dddddd #aaaaaa #333333", // focused
		"SolidRaised #cccccc #aaaaaa #333333", // unfocused
		"SolidRaised #bbbbbb #aaaaaa #333333", // pressed
		"SolidRaised #eeeeee #aaaaaa #333333" // hover
	};
	for (int i = 0; i < BUTTON_STATE_NO; i++) {
		if (_button[i] == nullptr) {
			_button[i] = _th->getTexture(button_textures[i]);
		}
	}
	if (_title_font == nullptr) {
		_title_font = _fh->getFont(DEFAULT_LARGE_FONT);
	}
	if (_title_color == nullptr) {
		_title_color = _fh->getColor("#000000");
	}
	if (_text_font == nullptr) {
		_text_font = _fh->getFont(DEFAULT_FONT);
	}
	if (_text_color == nullptr) {
		_text_color = _fh->getColor("#000000");
	}

	_loaded = true;
}

void
Theme::DialogData::clear()
{
	for (int i = 0; i < PAD_NO; i++) {
		_pad[i] = 2;
	}
}

/**
 * HarbourData constructor.
 */
Theme::HarbourData::HarbourData(TextureHandler* th)
	: _th(th),
	  _loaded(false),
	  _texture(nullptr)
{
}

/**
 * HarbourData destructor, unload data.
 */
Theme::HarbourData::~HarbourData(void)
{
	unload();
}

/**
 * Load harbour data and validate state, unloading previously loaded
 * data if any.
 */
bool
Theme::HarbourData::load(CfgParser::Entry *section)
{
	if (! section) {
		check();
		return false;
	}
	_loaded = true;

	CfgParser::Entry *value = section->findEntry("TEXTURE");
	if (value) {
		_texture = _th->getTexture(value->getValue());
	}

	check();

	return true;
}

/**
 * Unload harbour data.
 */
void
Theme::HarbourData::unload(void)
{
	if (! _loaded) {
		return;
	}
	_th->returnTexture(&_texture);
	_loaded = false;
}

/**
 * Check state of harbour data.
 */
void
Theme::HarbourData::check(void)
{
	if (! _texture) {
		_texture = _th->getTexture("Solid #000000");
	}

	_loaded = true;
}

// Theme

//! @brief Theme constructor
Theme::Theme(FontHandler *fh, ImageHandler *ih, TextureHandler *th,
	     const std::string& theme_file, const std::string &theme_variant,
	     bool is_owner)
	: _fh(fh),
	  _ih(ih),
	  _th(th),
	  _is_owner(is_owner),
	  _version(0),
	  _loaded(false),
	  _dialog_data(fh, th),
	  _menu_data(fh, th, 0),
	  _harbour_data(th),
	  _status_data(fh, th, 0),
	  _cmd_d_data(fh, th, 0),
	  _ws_indicator_data(fh, th)
{
	_decors[""] = nullptr;

	XGCValues gv;
	gv.function = GXinvert;
	gv.subwindow_mode = IncludeInferiors;
	gv.line_width = 1;
	ulong gv_mask = GCFunction|GCSubwindowMode|GCLineWidth;
	// cppcheck-suppress useInitializationList
	_invert_gc = X11::createGC(X11::getRoot(), gv_mask, &gv);

	load(theme_file, theme_variant);
}

//! @brief Theme destructor
Theme::~Theme(void)
{
	unload();
	X11::freeGC(_invert_gc);
}

/**
 * Re-loads theme if needed, clears up previously used resources.
 */
bool
Theme::load(const std::string &dir, const std::string &variant, bool force)
{
	std::string norm_dir, theme_file, theme_file_variant;
	getThemePaths(dir, variant, norm_dir, theme_file, theme_file_variant);
	if (! variant.empty()) {
		if (Util::isFile(theme_file_variant)) {
			theme_file = theme_file_variant;
		} else {
			P_DBG("theme variant " << variant << " does not exist");
		}
	}

	if (! force
	    && _theme_dir == norm_dir
	    && _theme_file == theme_file
	    && ! _cfg_files.requireReload(theme_file)) {
		return false;
	}

	unload();

	_theme_dir = norm_dir;
	_theme_file = theme_file;
	if (! _theme_dir.size()) {
		USER_WARN("empty theme directory name, using default");
		_theme_dir = DATADIR "/pekwm/themes/default";
		_theme_file = _theme_dir + "/theme";
	}

	bool theme_ok = true;
	CfgParserOpt opt(pekwm::configScriptPath());
	opt.setRegisterXResource(true);
	opt.setEndEarlyKey("REQUIRE");
	CfgParser theme(opt);
	theme.setVar("THEME_DIR", _theme_dir);
	if (! theme.parse(theme_file)) {
		_theme_dir = DATADIR "/pekwm/themes/default";
		theme.setVar("THEME_DIR", _theme_dir);
		theme_file = _theme_dir + "/theme";
		if (! theme.parse(theme_file)) {
			USER_WARN("unable to load " << _theme_dir
				  << " or default theme");
			theme_ok = false;
		}
	}

	if (theme_ok) {
		P_TRACE("Parsed theme: " << _theme_file);
	}

	if (_is_owner) {
		X11::setString(X11::getRoot(), PEKWM_THEME, theme_file);
	}

	// Setup quirks and requirements before parsing.
	if (theme_ok) {
		if (theme.isDynamicContent()) {
			_cfg_files.clear();
		} else {
			_cfg_files = theme.getCfgFiles();
		}
		ThemeUtil::loadRequire(theme, _theme_dir, theme_file);
	}
	CfgParser::Entry *root = theme.getEntryRoot();

	// Set image basedir.
	_ih->path_clear();
	_ih->path_push_back(_theme_dir + "/");

	loadVersion(root);
	loadBackground(root->findSection("BACKGROUND"));
	loadColorMaps(root->findSection("COLORMAPS"));

	_dialog_data.load(root->findSection("DIALOG"));
	loadDecors(root);

	if (! _menu_data.load(root->findSection("MENU"))) {
		P_WARN("Missing or malformed \"MENU\" section!");
	}
	if (! _status_data.load(root->findSection("STATUS"))) {
		P_WARN("Missing \"STATUS\" section!");
	}
	if (! _cmd_d_data.load(root->findSection("CMDDIALOG"))) {
		P_WARN("Missing \"CMDDIALOG\" section!");
	}
	CfgParser::Entry *ws_section = root->findSection("WORKSPACEINDICATOR");
	if (! _ws_indicator_data.load(ws_section)) {
		P_WARN("Missing \"WORKSPACEINDICATOR\" section!");
	}
	if (! _harbour_data.load(root->findSection("HARBOUR"))) {
		P_WARN("Missing \"HARBOUR\" section!");
	}

	_loaded = true;
	_th->logTextures("theme loaded");

	return true;
}

void
Theme::loadVersion(CfgParser::Entry* root)
{
	CfgParserKeys keys;
	keys.add_numeric<int>("VERSION", _version, 0);
	root->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	_menu_data.setVersion(_version);
	_cmd_d_data.setVersion(_version);
	_status_data.setVersion(_version);
}

void
Theme::loadBackground(CfgParser::Entry* section)
{
	_background = "";
	if (section == nullptr) {
		return;
	}

	CfgParserKeys keys;
	keys.add_string("TEXTURE", _background, "");
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();
}

void
Theme::loadColorMaps(CfgParser::Entry* section)
{
	_ih->clearColorMaps();
	if (section == nullptr) {
		return;
	}

	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		if (! pekwm::ascii_ncase_equal((*it)->getName(), "COLORMAP")) {
			USER_WARN("unexpected entry " << (*it)->getName()
				  << " in ColorMaps");
			continue;
		}

		std::map<int, int> color_map;
		Theme::ColorMap::load((*it)->getSection(), color_map);
		_ih->addColorMap((*it)->getValue(), color_map);
	}
}

void
Theme::loadDecors(CfgParser::Entry *root)
{
	CfgParser::Entry *section = root->findSection("PDECOR");
	if (section == nullptr) {
		section = root;
	}

	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		if (! pekwm::ascii_ncase_equal((*it)->getName(), "DECOR")) {
			continue;
		}

		PDecorData *data = new PDecorData(_fh, _th, _version);
		if (data->load((*it)->getSection())) {
			_decors[data->getName()] = data;
		} else {
			delete data;
		}
	}

	if (! getPDecorData("DEFAULT")) {
		// Create DEFAULT decor, let check fill it up with empty but
		// non-null data.
		P_WARN("Theme doesn't contain any DEFAULT decor.");
		PDecorData *decor_data =
			new PDecorData(_fh, _th, _version, "DEFAULT");
		decor_data->check();
		_decors["DEFAULT"] = decor_data;
	}
}

/**
 * Unload theme data.
 */
void
Theme::unload(void)
{
	if (! _loaded) {
		return;
	}
	_loaded = false;

	// Unload decors
	Util::StringMap<PDecorData*>::iterator it = _decors.begin();
	for (; it != _decors.end(); ++it) {
		delete it->second;
	}
	_decors.clear();
	_decors[""] = nullptr;

	// Unload theme data
	_dialog_data.unload();
	_menu_data.unload();
	_harbour_data.unload();
	_status_data.unload();
	_cmd_d_data.unload();
	_ws_indicator_data.unload();

	// Clear referenced colors
	X11::clearRefResources();

	_th->logTextures("theme unloaded");
}

/**
 * Return true if the theme at dir has a theme variant named variant.
 */
bool
Theme::variantExists(const std::string &dir, const std::string &variant)
{
	std::string norm_dir, file, variant_file;
	getThemePaths(dir, variant, norm_dir, file, variant_file);
	return Util::isFile(variant_file);
}

void
Theme::getThemePaths(const std::string &dir, const std::string &variant,
		     std::string &norm_dir, std::string &file,
		     std::string &variant_file)
{
	norm_dir = dir;
	if (dir.size() && dir.at(dir.size() - 1) == '/') {
		norm_dir.erase(norm_dir.end() - 1);
	}
	file = norm_dir + "/theme";
	variant_file = file + "-" + variant;
}
