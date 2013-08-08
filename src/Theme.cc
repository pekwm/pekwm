//
// Theme.cc for pekwm
// Copyright © 2003-2013 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::map;

// Theme::PDecorButtonData

//! @brief Theme::PDecorButtonData constructor.
Theme::PDecorButtonData::PDecorButtonData(void)
    : _shape(true), _left(false), _width(1), _height(1)
{
    for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
        _texture[i] = 0;
    }
}

//! @brief Theme::PDecorButtonData destructor.
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
        return false;
    }

    // Get actions.
    ActionEvent ae;
    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if (Config::instance()->parseActionEvent(*it, ae, BUTTONCLICK_OK, true)) {
            _aes.push_back (ae);
        }
    }

    // Got some actions, consider it to be a valid button.
    if (_aes.size() > 0) {
        TextureHandler *th = TextureHandler::instance();
        CfgParser::Entry *value;

        value = section->findEntry("SETSHAPE");
        if (value) {
            _shape = Util::isTrue(value->getValue());
        }

        value = section->findEntry("FOCUSED");
        if (value) {
            _texture[BUTTON_STATE_FOCUSED] = th->getTexture(value->getValue());
        }
        
        value = section->findEntry("UNFOCUSED");
        if (value) {
            _texture[BUTTON_STATE_UNFOCUSED] = th->getTexture(value->getValue());
        }
        
        value = section->findEntry("PRESSED");
        if (value) {
            _texture[BUTTON_STATE_PRESSED] = th->getTexture(value->getValue());
        }

        // HOOVER has been kept around due to backwards compatibility.
        value = section->findEntry("HOVER");
        if (! value) {
            value = section->findEntry("HOOVER");
        }
        if (value) {
            _texture[BUTTON_STATE_HOVER] = th->getTexture(value->getValue());
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
        TextureHandler::instance()->returnTexture(_texture[i]);
        _texture[i] = 0;
    }
}

//! @brief Verifies and makes sure no 0 textures exists.
void
Theme::PDecorButtonData::check(void)
{
    for (uint i = 0; i < (BUTTON_STATE_NO - 1); ++i) {
        if (! _texture[i]) {
            _texture[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }

    _width = _texture[BUTTON_STATE_FOCUSED]->getWidth();
    _height = _texture[BUTTON_STATE_FOCUSED]->getHeight();
}

// Theme::PDecorData

map<FocusedState, string> Theme::PDecorData::_fs_map = map<FocusedState, string>();
map<BorderPosition, string> Theme::PDecorData::_border_map = map<BorderPosition, string>();

//! @brief Theme::PDecorData constructor.
Theme::PDecorData::PDecorData(const char *name)
    : _title_height(0), _title_width_min(0), _title_width_max(100),
      _title_width_symetric(true), _title_height_adapt(false)
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
    unload();
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

    CfgParser::Entry *value;

    _name = section->getValue();
    if (! _name.size()) {
        cerr << " *** WARNING: no name identifying decor" << endl;
        return false;
    }

    CfgParser::Entry *title_section = section->findSection("TITLE");
    if (! title_section) {
        cerr << " *** WARNING: no title section in decor: " << _name << endl;
        return false;
    }

    TextureHandler *th = TextureHandler::instance(); // convenience

    vector<string> tok;
    vector<CfgParserKey*> keys;
    string value_pad, value_focused, value_unfocused;

    keys.push_back(new CfgParserKeyNumeric<int>("HEIGHT", _title_height, 10, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WIDTHMIN", _title_width_min, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WIDTHMAX", _title_width_max, 100, 0, 100));
    keys.push_back(new CfgParserKeyBool("WIDTHSYMETRIC", _title_width_symetric));
    keys.push_back(new CfgParserKeyBool("HEIGHTADAPT", _title_height_adapt));
    keys.push_back(new CfgParserKeyString("PAD", value_pad, "0 0 0 0", 7));
    keys.push_back(new CfgParserKeyString("FOCUSED", value_focused, "Empty", th->getLengthMin()));
    keys.push_back(new CfgParserKeyString("UNFOCUSED", value_unfocused, "Empty", th->getLengthMin()));

    // Free up resources
    title_section->parseKeyValues(keys.begin(), keys.end());

    for_each (keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    keys.clear();

    // Handle parsed data.
    _texture_main[FOCUSED_STATE_FOCUSED] = th->getTexture(value_focused);
    _texture_main[FOCUSED_STATE_UNFOCUSED] = th->getTexture(value_unfocused);
    if (Util::splitString(value_pad, tok, " \t", 4) == 4) {
        for (uint i = 0; i < PAD_NO; ++i)
            _pad[i] = strtol(tok[i].c_str(), 0, 10);
    }

    CfgParser::Entry *tab_section = title_section->findSection("TAB");
    if (tab_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            value = tab_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _texture_tab[i] = th->getTexture(value->getValue());
            }
        }
    }

    CfgParser::Entry *separator_section = title_section->findSection("SEPARATOR");
    if (separator_section) {
        keys.push_back(new CfgParserKeyString("FOCUSED", value_focused, "Empty", th->getLengthMin()));
        keys.push_back(new CfgParserKeyString("UNFOCUSED", value_unfocused, "Empty", th->getLengthMin()));

        // Parse data
        separator_section->parseKeyValues(keys.begin(), keys.end());

        // Free up resources
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
        keys.clear();

        // Handle parsed data.
        _texture_separator[FOCUSED_STATE_FOCUSED] = th->getTexture(value_focused);
        _texture_separator[FOCUSED_STATE_UNFOCUSED] = th->getTexture(value_unfocused);
    }


    CfgParser::Entry *font_section = title_section->findSection("FONT");
    if (font_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            value = font_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _font[i] = FontHandler::instance()->getFont(value->getValue());
            }
        }
    } else {
        WARN("no font section in decor: " << _name);
    }

    CfgParser::Entry *fontcolor_section = title_section->findSection("FONTCOLOR");
    if (fontcolor_section) {
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            value = fontcolor_section->findEntry(_fs_map[FocusedState(i)]);
            if (value) {
                _font_color[i] = FontHandler::instance()->getColor(value->getValue());
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
    TextureHandler *th = TextureHandler::instance();

    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        th->returnTexture(_texture_tab[i]);
        FontHandler::instance()->returnFont(_font[i]);
        FontHandler::instance()->returnColor(_font_color[i]);

        _texture_tab[i] = 0;
        _font[i] = 0;
        _font_color[i] = 0;
    }

    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        th->returnTexture(_texture_main[i]);
        th->returnTexture(_texture_separator[i]);
        _texture_main[i] = 0;
        _texture_separator[i] = 0;

        for (uint j = 0; j < BORDER_NO_POS; ++j) {
            th->returnTexture(_texture_border[i][j]);
            _texture_border[i][j] = 0;
        }
    }

    vector<Theme::PDecorButtonData*>::const_iterator it(_buttons.begin());
    for (; it != _buttons.end(); ++it) {
        delete *it;
    }
    _buttons.clear();
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

    TextureHandler *th = TextureHandler::instance(); // convenience

    CfgParser::Entry *sub, *value;

    sub = section->findSection("FOCUSED");
    if (sub) {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            value = sub->findEntry(_border_map[BorderPosition (i)]);
            if (value) {
                _texture_border[FOCUSED_STATE_FOCUSED][i] =
                    th->getTexture(value->getValue());
            }
        }
    }

    sub = section->findSection("UNFOCUSED");
    if (sub) {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            value = sub->findEntry(_border_map[BorderPosition (i)]);
            if (value) {
                _texture_border[FOCUSED_STATE_UNFOCUSED][i] =
                    th->getTexture(value->getValue());
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

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if (! (*it)->getSection()) {
            continue;
        }

        Theme::PDecorButtonData *btn = new Theme::PDecorButtonData();
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
            WARN(_name << " missing tab texture state " << _fs_map[FocusedState(i)]);
            _texture_tab[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }
    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        if (! _texture_main[i]) {
            WARN(_name << " missing main texture state " << _fs_map[FocusedState(i)]);
            _texture_main[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (! _texture_separator[i]) {
            WARN(_name << " missing tab texture state " << _fs_map[FocusedState(i)]);
            _texture_separator[i] = TextureHandler::instance()->getTexture("EMPTY");
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
        _font[FOCUSED_STATE_FOCUSED] = FontHandler::instance()->getFont("");
    }
}

//! @brief Checks for 0 border PTextures.
void
Theme::PDecorData::checkBorder(void)
{
    for (uint state = FOCUSED_STATE_FOCUSED; state < FOCUSED_STATE_FOCUSED_SELECTED; ++state) {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            if (! _texture_border[state][i]) {
                WARN(_name << " missing border texture "
                     << _border_map[BorderPosition(i)] << " "
                     << _fs_map[FocusedState(state)]);
                _texture_border[state][i] =
                    TextureHandler::instance()->getTexture("EMPTY");
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
            WARN(_name << " missing font color state " << _fs_map[FocusedState(i)]);
            _font_color[i] = FontHandler::instance()->getColor("#000000");
        }
    }
}

// Theme::PMenuData

//! @brief PMenuData constructor
Theme::PMenuData::PMenuData(void)
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

    CfgParser::Entry *value;
    value = section->findEntry("PAD");
    if (value) {
        vector<string> tok;
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
        FontHandler::instance()->returnFont(_font[i]);
        _font[i] = 0;

        FontHandler::instance()->returnColor(_color[i]);
        _color[i] = 0;

        TextureHandler::instance()->returnTexture(_tex_menu[i]);
        _tex_menu[i] = 0;

        TextureHandler::instance()->returnTexture(_tex_item[i]);
        _tex_item[i] = 0;

        TextureHandler::instance()->returnTexture(_tex_arrow[i]);
        _tex_arrow[i] = 0;
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        TextureHandler::instance()->returnTexture(_tex_sep[i]);
        _tex_sep[i] = 0;
    }
}

//! @brief Check data properties, prints warning and tries to fix.
void
Theme::PMenuData::check(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        if (! _font[i]) {
            _font[i] = FontHandler::instance()->getFont("");
        }
        if (! _color[i]) {
            _color[i] = FontHandler::instance()->getColor("#000000");
        }
        if (! _tex_menu[i]) {
            _tex_menu[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (! _tex_item[i]) {
            _tex_item[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (! _tex_arrow[i]) {
            _tex_arrow[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        if (! _tex_sep[i]) {
            _tex_sep[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }
}

//! @brief
void
Theme::PMenuData::loadState(CfgParser::Entry *section, ObjectState state)
{
    vector<CfgParserKey*> keys;
    string value_font, value_background, value_item;
    string value_text, value_arrow, value_separator;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("BACKGROUND", value_background, "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("ITEM", value_item, "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("TEXT", value_text, "Solid #000000"));
    keys.push_back(new CfgParserKeyString("ARROW", value_arrow, "Solid #000000"));
    if (state < OBJECT_STATE_SELECTED) {
        keys.push_back(new CfgParserKeyString("SEPARATOR", value_separator, "Solid #000000"));
    }

    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    TextureHandler *th = TextureHandler::instance();

    // Handle parsed data.
    _font[state] = FontHandler::instance()->getFont(value_font);
    _tex_menu[state] = th->getTexture(value_background);
    _tex_item[state] = th->getTexture(value_item);
    _color[state] = FontHandler::instance()->getColor(value_text);
    _tex_arrow[state] = th->getTexture(value_arrow);
    if (state < OBJECT_STATE_SELECTED) {
        _tex_sep[state] = th->getTexture(value_separator);
    }
}

// Theme::TextDialogData

//! @brief TextDialogData constructor.
Theme::TextDialogData::TextDialogData(void)
    : _font(0), _color(0), _tex(0)
{
    for (uint i = 0; i < PAD_NO; ++i) {
        _pad[i] = 0;
    }
}

//! @brief TextDialogData destructor.
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

    vector<CfgParserKey*> keys;
    string value_font, value_text, value_texture, value_pad;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("TEXT", value_text, "#000000"));
    keys.push_back(new CfgParserKeyString("TEXTURE", value_texture, "Solid #ffffff"));
    keys.push_back(new CfgParserKeyString("PAD", value_pad, "0 0 0 0", 7));

    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    // Handle parsed data.
    _font = FontHandler::instance()->getFont(value_font);
    _color = FontHandler::instance()->getColor(value_text);
    _tex = TextureHandler::instance()->getTexture(value_texture);

    vector<string> tok;
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
    FontHandler::instance()->returnFont(_font);
    FontHandler::instance()->returnColor(_color);
    TextureHandler::instance()->returnTexture(_tex);

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
        _font = FontHandler::instance()->getFont("");
    }
    if (! _color) {
        _color = FontHandler::instance()->getColor("#000000");
    }
    if (! _tex) {
        _tex = TextureHandler::instance()->getTexture("EMPTY");
    }
}

// WorkspaceIndicatorData

/**
 * WorkspaceIndicatorData constructor
 */
Theme::WorkspaceIndicatorData::WorkspaceIndicatorData(void)
    : font(0), font_color(0), texture_background(0),
      texture_workspace(0), texture_workspace_act(0),
      edge_padding(0), workspace_padding(0)
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

    vector<CfgParserKey*> keys;

    string value_font, value_color, value_tex_bg;
    string value_tex_ws, value_tex_ws_act;

    keys.push_back(new CfgParserKeyString("FONT", value_font));
    keys.push_back(new CfgParserKeyString("TEXT", value_color));
    keys.push_back(new CfgParserKeyString("BACKGROUND", value_tex_bg));
    keys.push_back(new CfgParserKeyString("WORKSPACE", value_tex_ws));
    keys.push_back(new CfgParserKeyString("WORKSPACEACTIVE", value_tex_ws_act));
    keys.push_back(new CfgParserKeyNumeric<int>("EDGEPADDING", edge_padding, 5, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WORKSPACEPADDING", workspace_padding, 2, 0));

    section->parseKeyValues(keys.begin(), keys.end());
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    font = FontHandler::instance()->getFont(value_font);
    font_color = FontHandler::instance()->getColor(value_color);
    texture_background = TextureHandler::instance()->getTexture(value_tex_bg);
    texture_workspace = TextureHandler::instance()->getTexture(value_tex_ws);
    texture_workspace_act = TextureHandler::instance()->getTexture(value_tex_ws_act);

    check();

    return true;
}

/**
 * Unload loaded theme data.
 */
void
Theme::WorkspaceIndicatorData::unload(void)
{
    FontHandler::instance()->returnFont(font);
    FontHandler::instance()->returnColor(font_color);
    TextureHandler::instance()->returnTexture(texture_background);
    TextureHandler::instance()->returnTexture(texture_workspace);
    TextureHandler::instance()->returnTexture(texture_workspace_act);

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
        font = FontHandler::instance()->getFont("Sans#Center#XFT");
    }
    
    if (! font_color) {
        font_color = FontHandler::instance()->getColor("#000000");
    }
    
    if (! texture_background) {
        texture_background = TextureHandler::instance()->getTexture("Solid #ffffff");
    }
    
    if (! texture_workspace) {
        texture_workspace = TextureHandler::instance()->getTexture("Solid #cccccc");
    }
    
    if (! texture_workspace_act) {
        texture_workspace_act = TextureHandler::instance()->getTexture("Solid #aaaaaa");
    }
}

/**
 * HarbourData constructor.
 */
Theme::HarbourData::HarbourData(void) : _texture(0)
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

    CfgParser::Entry *value;

    value = section->findEntry("TEXTURE");
    if (value) {
        _texture = TextureHandler::instance()->getTexture(value->getValue());
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
        TextureHandler::instance()->returnTexture(_texture);
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
        _texture = TextureHandler::instance()->getTexture("EMPTY");
    }
}

// Theme

//! @brief Theme constructor
Theme::Theme(void) : _is_loaded(false), _invert_gc(None)
{
    new ImageHandler();

    // window gc's
    XGCValues gv;
    gv.function = GXinvert;
    gv.subwindow_mode = IncludeInferiors;
    gv.line_width = 1;
    _invert_gc = XCreateGC(X11::getDpy(), X11::getRoot(),
                           GCFunction|GCSubwindowMode|GCLineWidth, &gv);

    X11::grabServer();

    load(Config::instance()->getThemeFile());

    X11::ungrabServer(true);
}

//! @brief Theme destructor
Theme::~Theme(void)
{
    unload(); // should clean things up

    XFreeGC(X11::getDpy(), _invert_gc);

    delete ImageHandler::instance();
}

/**
 * Re-loads theme if needed, clears up previously used resources.
 */
bool
Theme::load(const std::string &dir)
{
    string norm_dir(dir);
    if (dir.size() && dir.at(dir.size() - 1) != '/') {
        norm_dir.append("/");
    }
    string theme_file(norm_dir + string("theme"));

    if (! _cfg_files.requireReload(theme_file)) {
        return false;
    }

    if (_is_loaded) {
        unload();
    }

    _theme_dir = norm_dir;
    if (! _theme_dir.size()) {
        cerr << " *** WARNING: empty theme directory name, using default." << endl;
        _theme_dir = DATADIR "/pekwm/themes/default/";
    }

    bool theme_ok = true;
    CfgParser theme;

    if (! theme.parse(theme_file)) {
        _theme_dir = DATADIR "/pekwm/themes/default/";
        theme_file = _theme_dir + string("theme");
        if (! theme.parse(theme_file)) {
            cerr << " *** WARNING: couldn't load " << _theme_dir << " or default theme." << endl;
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

    // Set image basedir.
    ImageHandler::instance()->path_clear();
    ImageHandler::instance()->path_push_back(_theme_dir);

    // Load decor data.
    CfgParser::Entry *section = theme.getEntryRoot()->findSection("PDECOR");
    if (section) {
        CfgParser::iterator it(section->begin());
        for (; it != section->end(); ++it) {
            Theme::PDecorData *data = new Theme::PDecorData();
            if (data->load((*it)->getSection())) {
                _pdecordata_map[data->getName()] = data;
            } else {
                delete data;
            }
        }
    }

    if (! getPDecorData("DEFAULT")) {
        // Create DEFAULT decor, let check fill it up with empty but non-null data.
        WARN("Theme doesn't contain any DEFAULT decor.");
        Theme::PDecorData *decor_data = new Theme::PDecorData("DEFAULT");
        decor_data->check();
        _pdecordata_map["DEFAULT"] = decor_data;
    }

    if (! _menu_data.load(theme.getEntryRoot()->findSection("MENU"))) {
        WARN("Missing or malformed \"MENU\" section!");
    }

    if (! _status_data.load(theme.getEntryRoot()->findSection("STATUS"))) {
        WARN("Missing \"STATUS\" section!");
    }

    if (! _cmd_d_data.load(theme.getEntryRoot()->findSection("CMDDIALOG"))) {
        WARN("Missing \"CMDDIALOG\" section!");
    }

    if (! _ws_indicator_data.load(theme.getEntryRoot()->findSection("WORKSPACEINDICATOR"))) {
        WARN("Missing \"WORKSPACEINDICATOR\" section!");
    }

    if (! _harbour_data.load(theme.getEntryRoot()->findSection("HARBOUR"))) {
        WARN("Missing \"HARBOUR\" section!");
    }

    _is_loaded = true;

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
        vector<CfgParserKey*> keys;
        bool value_templates;

        keys.push_back(new CfgParserKeyBool("TEMPLATES", value_templates, false));
        section->parseKeyValues(keys.begin(), keys.end());
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

        // Re-load configuration with templates enabled.
        if (value_templates) {
            theme_cfg.clear(true);
            theme_cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
        }
    }
}

/**
 * Unload theme data.
 */
void
Theme::unload(void)
{
    // Unload decors
    map<string, Theme::PDecorData*>::iterator p_it(_pdecordata_map.begin());
    for (; p_it != _pdecordata_map.end(); ++p_it) {
        delete p_it->second;
    }
    _pdecordata_map.clear();

    // Unload theme data
    _menu_data.unload();
    _status_data.unload();
    _cmd_d_data.unload();
    _ws_indicator_data.unload();
    _harbour_data.unload();

    _is_loaded = false;
}
