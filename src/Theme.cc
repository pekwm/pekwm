//
// Theme.cc for pekwm
// Copyright (C) 2003-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "ParseUtil.hh"
#include "Theme.hh"

#include "Debug.hh"
#include "x11.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PFont.hh"
#include "PTexture.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "TextureHandler.hh"
#include "Util.hh"

#include <iostream>
#include <cstdlib>

// Theme::PDecorButtonData

//! @brief Theme::PDecorButtonData constructor.
Theme::PDecorButtonData::PDecorButtonData(TextureHandler *th)
    : _th(th),
      _loaded(false),
      _shape(true),
      _left(false),
      _width(1),
      _height(1)
{
    for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
        _texture[i] = 0;
    }
}

//! @brief Theme::PDecorButtonData destructor.
Theme::PDecorButtonData::~PDecorButtonData(void)
{
    if (_loaded) {
        unload();
    }
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
        return false;
    }
    _loaded = true;

    // Get actions.
    ActionEvent ae;
    auto it(section->begin());
    for (; it != section->end(); ++it) {
        if (pekwm::config()->parseActionEvent(*it, ae, BUTTONCLICK_OK, true)) {
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
            _texture[BUTTON_STATE_FOCUSED] = _th->getTexture(value->getValue());
        }
        value = section->findEntry("UNFOCUSED");
        if (value) {
            _texture[BUTTON_STATE_UNFOCUSED] =
                _th->getTexture(value->getValue());
        }
        value = section->findEntry("PRESSED");
        if (value) {
            _texture[BUTTON_STATE_PRESSED] = _th->getTexture(value->getValue());
        }

        // HOOVER has been kept around due to backwards compatibility.
        value = section->findEntry("HOVER");
        if (! value) {
            value = section->findEntry("HOOVER");
        }
        if (value) {
            _texture[BUTTON_STATE_HOVER] = _th->getTexture(value->getValue());
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
    for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
        _th->returnTexture(_texture[i]);
        _texture[i] = 0;
    }
    _loaded = false;
}

//! @brief Verifies and makes sure no 0 textures exists.
void
Theme::PDecorButtonData::check(void)
{
    for (uint i = 0; i < (BUTTON_STATE_NO - 1); ++i) {
        if (! _texture[i]) {
            _texture[i] = _th->getTexture("EMPTY");
        }
    }

    _width = _texture[BUTTON_STATE_FOCUSED]->getWidth();
    _height = _texture[BUTTON_STATE_FOCUSED]->getHeight();
}

// Theme::PDecorData

std::map<FocusedState, std::string> Theme::PDecorData::_fs_map =
    std::map<FocusedState, std::string>();
std::map<BorderPosition, std::string> Theme::PDecorData::_border_map =
    std::map<BorderPosition, std::string>();

//! @brief Theme::PDecorData constructor.
Theme::PDecorData::PDecorData(FontHandler* fh, TextureHandler* th,
                              const char *name)
    : _fh(fh),
      _th(th),
      _loaded(false),
      _title_height(10),
      _title_width_min(0),
      _title_width_max(100),
      _title_width_symetric(true),
      _title_height_adapt(false)
{
    if (name) {
        _name = name;
    }

    // init static data
    if (! _fs_map.size()) {
        _fs_map[FOCUSED_STATE_FOCUSED] = "FOCUSED";
        _fs_map[FOCUSED_STATE_UNFOCUSED] = "UNFOCUSED";
        _fs_map[FOCUSED_STATE_FOCUSED_SELECTED] = "FOCUSEDSELECTED";
        _fs_map[FOCUSED_STATE_UNFOCUSED_SELECTED] = "UNFOCUSEDSELECTED";
    }
    if (! _border_map.size()) {
        _border_map[BORDER_TOP_LEFT] = "TOPLEFT";
        _border_map[BORDER_TOP] = "TOP";
        _border_map[BORDER_TOP_RIGHT] = "TOPRIGHT";
        _border_map[BORDER_LEFT] = "LEFT";
        _border_map[BORDER_RIGHT] = "RIGHT";
        _border_map[BORDER_BOTTOM_LEFT] = "BOTTOMLEFT";
        _border_map[BORDER_BOTTOM] = "BOTTOM";
        _border_map[BORDER_BOTTOM_RIGHT] = "BOTTOMRIGHT";
    }

    // init arrays
    for (uint i = 0; i < PAD_NO; ++i) {
        _pad[i] = 0;
    }
    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        _texture_tab[i] = 0;
        _font[i] = 0;
        _font_color[i] = 0;
    }
    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        _texture_main[i] = 0;
        _texture_separator[i] = 0;
    }

    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        for (uint j = 0; j < BORDER_NO_POS; ++j) {
            _texture_border[i][j] = 0;
        }
    }
}

//! @brief Theme::PDecorData destructor.
Theme::PDecorData::~PDecorData(void)
{
    if (_loaded) {
        unload();
    }
}

//! @brief Parses CfgParser::Entry section, loads and verifies data.
//! @param section CfgParser::Entry with pdecor configuration.
//! @return True if a valid pdecor was parsed.
bool
Theme::PDecorData::load(CfgParser::Entry *section)
{
    if (! section) {
        check();
        return false;
    }

    _name = section->getValue();
    if (! _name.size()) {
        std::cerr << " *** WARNING: no name identifying decor" << std::endl;
        return false;
    }

    CfgParser::Entry *title_section = section->findSection("TITLE");
    if (! title_section) {
        std::cerr << " *** WARNING: no title section in decor: " << _name
                  << std::endl;
        return false;
    }
    _loaded = true;

    std::vector<std::string> tok;
    std::vector<CfgParserKey*> keys;
    std::string value_pad, value_focused, value_unfocused;

    keys.push_back(new CfgParserKeyNumeric<int>("HEIGHT",
                                                _title_height, 10, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WIDTHMIN",
                                                _title_width_min, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WIDTHMAX",
                                                _title_width_max, 100, 0, 100));
    keys.push_back(new CfgParserKeyBool("WIDTHSYMETRIC",
                                        _title_width_symetric));
    keys.push_back(new CfgParserKeyBool("HEIGHTADAPT", _title_height_adapt));
    keys.push_back(new CfgParserKeyString("PAD", value_pad, "0 0 0 0", 7));
    keys.push_back(new CfgParserKeyString("FOCUSED", value_focused,
                                          "Empty", _th->getLengthMin()));
    keys.push_back(new CfgParserKeyString("UNFOCUSED", value_unfocused,
                                          "Empty", _th->getLengthMin()));

    // Free up resources
    title_section->parseKeyValues(keys.begin(), keys.end());

    for_each (keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    keys.clear();

    // Handle parsed data.
    _texture_main[FOCUSED_STATE_FOCUSED] = _th->getTexture(value_focused);
    _texture_main[FOCUSED_STATE_UNFOCUSED] = _th->getTexture(value_unfocused);
    if (Util::splitString(value_pad, tok, " \t", 4) == 4) {
        for (uint i = 0; i < PAD_NO; ++i)
            _pad[i] = strtol(tok[i].c_str(), 0, 10);
    }

    auto tab_section = title_section->findSection("TAB");
    if (tab_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            auto value = tab_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _texture_tab[i] = _th->getTexture(value->getValue());
            }
        }
    }

    auto separator_section = title_section->findSection("SEPARATOR");
    if (separator_section) {
        keys.push_back(new CfgParserKeyString("FOCUSED", value_focused,
                                              "Empty", _th->getLengthMin()));
        keys.push_back(new CfgParserKeyString("UNFOCUSED", value_unfocused,
                                              "Empty", _th->getLengthMin()));

        // Parse data
        separator_section->parseKeyValues(keys.begin(), keys.end());

        // Free up resources
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
        keys.clear();

        // Handle parsed data.
        _texture_separator[FOCUSED_STATE_FOCUSED] =
            _th->getTexture(value_focused);
        _texture_separator[FOCUSED_STATE_UNFOCUSED] =
            _th->getTexture(value_unfocused);
    }


    auto font_section = title_section->findSection("FONT");
    if (font_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            auto value = font_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _font[i] = _fh->getFont(value->getValue());
            }
        }
    } else {
        WARN("no font section in decor: " << _name);
    }

    auto fontcolor_section = title_section->findSection("FONTCOLOR");
    if (fontcolor_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            auto value = fontcolor_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _font_color[i] = _fh->getColor(value->getValue());
            }
        }
    }

    loadButtons(title_section->findSection("BUTTONS"));
    loadBorder(title_section->findSection("BORDER"));

    check();

    return true;
}

//! @brief Unloads data.
void
Theme::PDecorData::unload(void)
{
    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        _th->returnTexture(_texture_tab[i]);
        _fh->returnFont(_font[i]);
        _fh->returnColor(_font_color[i]);

        _texture_tab[i] = 0;
        _font[i] = 0;
        _font_color[i] = 0;
    }

    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        _th->returnTexture(_texture_main[i]);
        _th->returnTexture(_texture_separator[i]);
        _texture_main[i] = 0;
        _texture_separator[i] = 0;

        for (uint j = 0; j < BORDER_NO_POS; ++j) {
            _th->returnTexture(_texture_border[i][j]);
            _texture_border[i][j] = 0;
        }
    }

    for (auto it : _buttons) {
        delete it;
    }
    _buttons.clear();

    _loaded = false;
}

//! @brief Checks data properties, prints warning and tries to fix.
void
Theme::PDecorData::check(void)
{
    // check values
    if (_title_width_max > 100) {
        WARN(_name << " WIDTHMAX > 100");
        _title_width_max = 100;
    }

    checkTextures();
    checkFonts();
    checkBorder();
    checkColors();
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
            value = sub->findEntry(_border_map[BorderPosition (i)]);
            if (value) {
                _texture_border[FOCUSED_STATE_FOCUSED][i] =
                    _th->getTexture(value->getValue());
            }
        }
    }

    sub = section->findSection("UNFOCUSED");
    if (sub) {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            value = sub->findEntry(_border_map[BorderPosition (i)]);
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

    auto it(section->begin());
    for (; it != section->end(); ++it) {
        if (! (*it)->getSection()) {
            continue;
        }

        auto btn = new Theme::PDecorButtonData(_th);
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
    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        if (! _texture_tab[i]) {
            WARN(_name << " missing tab texture state "
                 << _fs_map[FocusedState(i)]);
            _texture_tab[i] = _th->getTexture("EMPTY");
        }
    }
    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        if (! _texture_main[i]) {
            WARN(_name << " missing main texture state "
                 << _fs_map[FocusedState(i)]);
            _texture_main[i] = _th->getTexture("EMPTY");
        }
        if (! _texture_separator[i]) {
            WARN(_name << " missing tab texture state "
                 << _fs_map[FocusedState(i)]);
            _texture_separator[i] = _th->getTexture("EMPTY");
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
        WARN(_name << " missing font state " << _fs_map[FOCUSED_STATE_FOCUSED]);
        _font[FOCUSED_STATE_FOCUSED] = _fh->getFont("");
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
                WARN(_name << " missing border texture "
                     << _border_map[BorderPosition(i)] << " "
                     << _fs_map[FocusedState(state)]);
                _texture_border[state][i] =
                    _th->getTexture("EMPTY");
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
            WARN(_name << " missing font color state "
                 << _fs_map[FocusedState(i)]);
            _font_color[i] = _fh->getColor("#000000");
        }
    }
}

// Theme::PMenuData

//! @brief PMenuData constructor
Theme::PMenuData::PMenuData(FontHandler* fh, TextureHandler* th)
    : _fh(fh),
      _th(th)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        _font[i] = 0;
        _color[i] = 0;
        _tex_menu[i] = 0;
        _tex_item[i] = 0;
        _tex_arrow[i] = 0;
    }
    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        _tex_sep[i] = 0;
    }
    for (uint i = 0; i < PAD_NO; ++i) {
        _pad[i] = 0;
    }
}

//! @brief PMenuData destructor
Theme::PMenuData::~PMenuData(void)
{
    if (_loaded) {
        unload();
    }
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

    CfgParser::Entry *value;
    value = section->findEntry("PAD");
    if (value) {
        std::vector<std::string> tok;
        if (Util::splitString (value->getValue(), tok, " \t", 4) == 4) {
            for (int i = 0; i < PAD_NO; ++i) {
                _pad[i] = strtol (tok[i].c_str(), 0, 10);
            }
        }
    }

    value = section->findSection("FOCUSED");
    if (value) {
        loadState(value, OBJECT_STATE_FOCUSED);
    }

    value = section->findSection("UNFOCUSED");
    if (value) {
        loadState(value, OBJECT_STATE_UNFOCUSED);
    }

    value = section->findSection("SELECTED");
    if (value) {
        loadState(value, OBJECT_STATE_SELECTED);
    }

    check();

    return true;
}

//! @brief Unloads data.
void
Theme::PMenuData::unload(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        _fh->returnFont(_font[i]);
        _font[i] = 0;

        _fh->returnColor(_color[i]);
        _color[i] = 0;

        _th->returnTexture(_tex_menu[i]);
        _tex_menu[i] = 0;

        _th->returnTexture(_tex_item[i]);
        _tex_item[i] = 0;

        _th->returnTexture(_tex_arrow[i]);
        _tex_arrow[i] = 0;
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        _th->returnTexture(_tex_sep[i]);
        _tex_sep[i] = 0;
    }
}

//! @brief Check data properties, prints warning and tries to fix.
void
Theme::PMenuData::check(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        if (! _font[i]) {
            _font[i] = _fh->getFont("");
        }
        if (! _color[i]) {
            _color[i] = _fh->getColor("#000000");
        }
        if (! _tex_menu[i]) {
            _tex_menu[i] = _th->getTexture("EMPTY");
        }
        if (! _tex_item[i]) {
            _tex_item[i] = _th->getTexture("EMPTY");
        }
        if (! _tex_arrow[i]) {
            _tex_arrow[i] = _th->getTexture("EMPTY");
        }
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        if (! _tex_sep[i]) {
            _tex_sep[i] = _th->getTexture("EMPTY");
        }
    }
}

void
Theme::PMenuData::loadState(CfgParser::Entry *section, ObjectState state)
{
    std::vector<CfgParserKey*> keys;
    std::string value_font, value_background, value_item;
    std::string value_text, value_arrow, value_separator;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("BACKGROUND", value_background,
                                          "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("ITEM", value_item, "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("TEXT", value_text, "Solid #000000"));
    keys.push_back(new CfgParserKeyString("ARROW", value_arrow,
                                          "Solid #000000"));
    if (state < OBJECT_STATE_SELECTED) {
        keys.push_back(new CfgParserKeyString("SEPARATOR", value_separator,
                                              "Solid #000000"));
    }

    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

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

//! @brief TextDialogData constructor.
Theme::TextDialogData::TextDialogData(FontHandler* fh, TextureHandler* th)
    : _fh(fh),
      _th(th),
      _font(0),
      _color(0),
      _tex(0)
{
    for (uint i = 0; i < PAD_NO; ++i) {
        _pad[i] = 0;
    }
}

//! @brief TextDialogData destructor.
Theme::TextDialogData::~TextDialogData(void)
{
    if (_loaded) {
        unload();
    }
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

    std::vector<CfgParserKey*> keys;
    std::string value_font, value_text, value_texture, value_pad;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("TEXT", value_text, "#000000"));
    keys.push_back(new CfgParserKeyString("TEXTURE", value_texture,
                                          "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("PAD", value_pad, "0 0 0 0", 7));

    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    // Handle parsed data.
    _font = _fh->getFont(value_font);
    _color = _fh->getColor(value_text);
    _tex = _th->getTexture(value_texture);

    std::vector<std::string> tok;
    if (Util::splitString(value_pad, tok, " \t", 4) == 4) {
        for (uint i = 0; i < PAD_NO; ++i) {
            _pad[i] = strtol(tok[i].c_str(), 0, 10);
        }
    }

    check();

    return true;
}

//! @brief Unloads data.
void
Theme::TextDialogData::unload(void)
{
    _fh->returnFont(_font);
    _fh->returnColor(_color);
    _th->returnTexture(_tex);

    _font = 0;
    _tex = 0;
    _color = 0;
}

//! @brief Check data properties, prints warning and tries to fix.
//! @todo print warnings
void
Theme::TextDialogData::check(void)
{
    if (! _font) {
        _font = _fh->getFont("");
    }
    if (! _color) {
        _color = _fh->getColor("#000000");
    }
    if (! _tex) {
        _tex = _th->getTexture("EMPTY");
    }
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
      font(0),
      font_color(0),
      texture_background(0),
      texture_workspace(0),
      texture_workspace_act(0),
      edge_padding(0),
      workspace_padding(0)
{
}

/**
 * WorkspaceIndicatorData destructor
 */
Theme::WorkspaceIndicatorData::~WorkspaceIndicatorData(void)
{
    if (_loaded) {
        unload();
    }
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

    std::vector<CfgParserKey*> keys;

    std::string value_font, value_color, value_tex_bg;
    std::string value_tex_ws, value_tex_ws_act;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("TEXT", value_color));
    keys.push_back(new CfgParserKeyString("BACKGROUND", value_tex_bg));
    keys.push_back(new CfgParserKeyString("WORKSPACE", value_tex_ws));
    keys.push_back(new CfgParserKeyString("WORKSPACEACTIVE", value_tex_ws_act));
    keys.push_back(new CfgParserKeyNumeric<int>("EDGEPADDING", edge_padding,
                                                5, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WORKSPACEPADDING", workspace_padding,
                                                2, 0));

    section->parseKeyValues(keys.begin(), keys.end());
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

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
    _fh->returnFont(font);
    _fh->returnColor(font_color);
    _th->returnTexture(texture_background);
    _th->returnTexture(texture_workspace);
    _th->returnTexture(texture_workspace_act);

    font = 0;
    font_color = 0;
    texture_background = 0;
    texture_workspace = 0;
    texture_workspace_act = 0;
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
        font = _fh->getFont("Sans#Center#XFT");
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
}

/**
 * HarbourData constructor.
 */
Theme::HarbourData::HarbourData(TextureHandler* th)
    : _th(th),
      _texture(0)
{
}

/**
 * HarbourData destructor, unload data.
 */
Theme::HarbourData::~HarbourData(void)
{
    if (_loaded) {
        unload();
    }
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

    CfgParser::Entry *value;

    value = section->findEntry("TEXTURE");
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
    if (_texture) {
        _th->returnTexture(_texture);
        _texture = 0;
    }
}


/**
 * Check state of harbour data.
 */
void
Theme::HarbourData::check(void)
{
    if (! _texture) {
        _texture = _th->getTexture("EMPTY");
    }
}

// Theme

//! @brief Theme constructor
Theme::Theme(FontHandler *fh, ImageHandler *ih, TextureHandler *th)
    : _fh(fh),
      _ih(ih),
      _th(th),
      _loaded(false),
      _invert_gc(None),
      _bg_pid(-1),
      _menu_data(fh, th),
      _harbour_data(th),
      _status_data(fh, th),
      _cmd_d_data(fh, th),
      _ws_indicator_data(fh, th)
{
    // window gc's
    XGCValues gv;
    gv.function = GXinvert;
    gv.subwindow_mode = IncludeInferiors;
    gv.line_width = 1;
    _invert_gc = XCreateGC(X11::getDpy(), X11::getRoot(),
                           GCFunction|GCSubwindowMode|GCLineWidth, &gv);

    X11::grabServer();
    load(pekwm::config()->getThemeFile());
    X11::ungrabServer(true);
}

//! @brief Theme destructor
Theme::~Theme(void)
{
    unload();
    XFreeGC(X11::getDpy(), _invert_gc);
}

/**
 * Re-loads theme if needed, clears up previously used resources.
 */
bool
Theme::load(const std::string &dir)
{
    std::string norm_dir(dir);
    if (dir.size() && dir.at(dir.size() - 1) != '/') {
        norm_dir.append("/");
    }
    std::string theme_file(norm_dir + "theme");

    if (! _cfg_files.requireReload(theme_file)) {
        return false;
    }

    if (_loaded) {
        unload();
    }

    _theme_dir = norm_dir;
    if (! _theme_dir.size()) {
        std::cerr << " *** WARNING: empty theme directory name, using default."
                  << std::endl;
        _theme_dir = DATADIR "/pekwm/themes/default/";
    }

    bool theme_ok = true;
    CfgParser theme;

    if (! theme.parse(theme_file)) {
        _theme_dir = DATADIR "/pekwm/themes/default/";
        theme_file = _theme_dir + std::string("theme");
        if (! theme.parse(theme_file)) {
            std::cerr << " *** WARNING: couldn't load " << _theme_dir
                      << " or default theme." << std::endl;
            theme_ok = false;
        }
    }

    // Setup quirks and requirements before parsing.
    if (theme_ok) {
        if (theme.isDynamicContent()) {
            _cfg_files.clear();
        } else {
            _cfg_files = theme.getCfgFiles();
        }
        loadThemeRequire(theme, theme_file);
    }
    auto root = theme.getEntryRoot();

    // Set image basedir.
    _ih->path_clear();
    _ih->path_push_back(_theme_dir);

    loadBackground(root->findSection("BACKGROUND"));

    // Load decor data.
    auto section = root->findSection("PDECOR");
    if (section) {
        auto it(section->begin());
        for (; it != section->end(); ++it) {
            auto data = new Theme::PDecorData(_fh, _th);
            if (data->load((*it)->getSection())) {
                _pdecordata_map[data->getName()] = data;
            } else {
                delete data;
            }
        }
    }

    if (! getPDecorData("DEFAULT")) {
        // Create DEFAULT decor, let check fill it up with empty but
        // non-null data.
        WARN("Theme doesn't contain any DEFAULT decor.");
        auto decor_data = new Theme::PDecorData(_fh, _th, "DEFAULT");
        decor_data->check();
        _pdecordata_map["DEFAULT"] = decor_data;
    }

    if (! _menu_data.load(root->findSection("MENU"))) {
        WARN("Missing or malformed \"MENU\" section!");
    }

    if (! _status_data.load(root->findSection("STATUS"))) {
        WARN("Missing \"STATUS\" section!");
    }

    if (! _cmd_d_data.load(root->findSection("CMDDIALOG"))) {
        WARN("Missing \"CMDDIALOG\" section!");
    }

    auto ws_section = root->findSection("WORKSPACEINDICATOR");
    if (! _ws_indicator_data.load(ws_section)) {
        WARN("Missing \"WORKSPACEINDICATOR\" section!");
    }

    if (! _harbour_data.load(root->findSection("HARBOUR"))) {
        WARN("Missing \"HARBOUR\" section!");
    }

    _loaded = true;

    return true;
}

/**
 * Load template quirks.
 */
void
Theme::loadThemeRequire(CfgParser &theme_cfg, std::string &file)
{
    CfgParser::Entry *section;

    // Look for requires section,
    section = theme_cfg.getEntryRoot()->findSection("REQUIRE");
    if (section) {
        std::vector<CfgParserKey*> keys;
        bool value_templates;

        keys.push_back(new CfgParserKeyBool("TEMPLATES", value_templates,
                                            false));
        section->parseKeyValues(keys.begin(), keys.end());
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

        // Re-load configuration with templates enabled.
        if (value_templates) {
            theme_cfg.clear(true);
            theme_cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
        }
    }
}

void
Theme::loadBackground(CfgParser::Entry* section)
{
    if (section == nullptr) {
        return;
    }

    std::vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyString("TEXTURE", _background, ""));
    section->parseKeyValues(keys.begin(), keys.end());
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    if (! _background.empty()
        && pekwm::config()->getThemeBackground()) {
        startBackground(_background);
    }
}

void
Theme::startBackground(const std::string& texture)
{
    if (_bg_pid != -1) {
        stopBackground();
    }

    std::vector<std::string> args =
        {BINDIR "/pekwm_bg",
         "--load-dir", _theme_dir + "/backgrounds",
         texture};
    _bg_pid = Util::forkExec(args);
}

void
Theme::stopBackground(void)
{
    // SIGCHILD will take care of waiting for the child
    kill(_bg_pid, SIGKILL);
    _bg_pid = -1;
}

/**
 * Unload theme data.
 */
void
Theme::unload(void)
{
    // Unload decors
    for (auto it : _pdecordata_map) {
        delete it.second;
    }
    _pdecordata_map.clear();

    // Unload theme data
    _menu_data.unload();
    _status_data.unload();
    _cmd_d_data.unload();
    _ws_indicator_data.unload();
    _harbour_data.unload();

    _loaded = false;
}
