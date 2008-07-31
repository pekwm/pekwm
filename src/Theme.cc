//
// Theme.cc for pekwm
// Copyright © 2003-2008 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "ParseUtil.hh"
#include "Theme.hh"

#include "PScreen.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PFont.hh"
#include "PTexture.hh"
#include "ColorHandler.hh"
#include "FontHandler.hh"
#include "ImageHandler.hh"
#include "TextureHandler.hh"
#include "Util.hh"

#include <iostream>
#include <cstdlib>

using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::vector;
using std::map;

// Theme::PDecorButtonData

//! @brief Theme::PDecorButtonData constructor.
Theme::PDecorButtonData::PDecorButtonData(void) :
        _left(false), _width(1), _height(1)
{
    for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
        _texture[i] = NULL;
    }
}

//! @brief Theme::PDecorButtonData destructor.
Theme::PDecorButtonData::~PDecorButtonData(void)
{
    unload();
}

//! @brief Parses CfgParser::Entry op_section, loads and verifies data.
//! @param op_section CfgParser::Entry with button configuration.
//! @return True if a valid button was parsed.
bool
Theme::PDecorButtonData::load (CfgParser::Entry *op_section)
{
    if (*op_section == "LEFT") {
        _left = true;
    } else if (*op_section == "RIGHT") {
        _left = false;
    } else {
        return false;
    }
    
    op_section = op_section->get_section ();

    // Get actions.
    ActionEvent ae;
    CfgParser::Entry *op_it;
    for (op_it = op_section->get_section_next (); op_it; op_it = op_it->get_section_next ()) {
        if (Config::instance ()->parseActionEvent (op_it, ae, BUTTONCLICK_OK, true)) {
            _ae_list.push_back (ae);
        }
    }

    // Got some actions, consider it to be a valid button.
    if (_ae_list.size() > 0) {
        TextureHandler *th = TextureHandler::instance();
        CfgParser::Entry *op_value;

        op_value = op_section->find_entry ("FOCUSED");
        if (op_value) {
            _texture[BUTTON_STATE_FOCUSED] = th->getTexture (op_value->get_value ());
        }
        
        op_value = op_section->find_entry ("UNFOCUSED");
        if (op_value) {
            _texture[BUTTON_STATE_UNFOCUSED] = th->getTexture (op_value->get_value ());
        }
        
        op_value = op_section->find_entry ("PRESSED");
        if (op_value) {
            _texture[BUTTON_STATE_PRESSED] = th->getTexture (op_value->get_value ());
        }
        
        op_value = op_section->find_entry ("HOOVER");
        if (op_value) {
            _texture[BUTTON_STATE_HOOVER] = th->getTexture (op_value->get_value ());
        }
        
        check ();

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
        _texture[i] = NULL;
    }
}

//! @brief Verifies and makes sure no NULL textures exists.
void
Theme::PDecorButtonData::check(void)
{
    for (uint i = 0; i < BUTTON_STATE_NO; ++i) {
        if (!_texture[i]) {
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
Theme::PDecorData::PDecorData(void) :
        _title_height(0), _title_width_min(0), _title_width_max(100),
        _title_width_symetric(true), _title_height_adapt(false)
{
    // init static data
    if (!_fs_map.size()) {
        _fs_map[FOCUSED_STATE_FOCUSED] = "FOCUSED";
        _fs_map[FOCUSED_STATE_UNFOCUSED] = "UNFOCUSED";
        _fs_map[FOCUSED_STATE_FOCUSED_SELECTED] = "FOCUSEDSELECTED";
        _fs_map[FOCUSED_STATE_UNFOCUSED_SELECTED] = "UNFOCUSEDSELECTED";
    }
    if (!_border_map.size()) {
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
        _texture_tab[i] = NULL;
        _font[i] = NULL;
        _font_color[i] = NULL;
    }
    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        _texture_main[i] = NULL;
        _texture_separator[i] = NULL;
    }

    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        for (uint j = 0; j < BORDER_NO_POS; ++j) {
            _texture_border[i][j] = NULL;
        }
    }
}

//! @brief Theme::PDecorData destructor.
Theme::PDecorData::~PDecorData(void)
{
    unload();
}

//! @brief Parses CfgParser::Entry op_section, loads and verifies data.
//! @param op_section CfgParser::Entry with pdecor configuration.
//! @return True if a valid pdecor was parsed.
bool
Theme::PDecorData::load (CfgParser::Entry *op_section)
{
    CfgParser::Entry *op_value;

    _name = op_section->get_value ();
    if (!_name.size ()) {
        cerr << " *** WARNING: no name identifying decor" << endl;
        return false;
    }

    op_section = op_section->get_section ();

    CfgParser::Entry *op_sub, *op_sub_2;
    op_sub = op_section->find_section ("TITLE");
    if (!op_sub) {
        cerr << " *** WARNING: no title section in decor: " << _name << endl;
        return false;
    }
    op_sub = op_sub->get_section ();

    TextureHandler *th = TextureHandler::instance(); // convenience

    vector<string> tok;
    list<CfgParserKey*> o_key_list;
    string o_value_pad, o_value_focused, o_value_unfocused;

    o_key_list.push_back (new CfgParserKeyInt ("HEIGHT", _title_height, 10, 0));
    o_key_list.push_back (new CfgParserKeyInt ("WIDTHMIN", _title_width_min, 0));
    o_key_list.push_back (new CfgParserKeyInt ("WIDTHMAX", _title_width_max,
                          100, 0, 100));
    o_key_list.push_back (new CfgParserKeyBool ("WIDTHSYMETRIC", _title_width_symetric));
    o_key_list.push_back (new CfgParserKeyBool ("HEIGHTADAPT", _title_height_adapt));
    o_key_list.push_back (new CfgParserKeyString ("PAD", o_value_pad, "0 0 0 0", 7));
    o_key_list.push_back (new CfgParserKeyString ("FOCUSED", o_value_focused,
                          "Empty", th->getLengthMin ()));
    o_key_list.push_back (new CfgParserKeyString ("UNFOCUSED", o_value_unfocused,
                          "Empty", th->getLengthMin ()));
    // Free up resources
    op_sub->parse_key_values (o_key_list.begin (), o_key_list.end ());

    for_each (o_key_list.begin (), o_key_list.end (),
              Util::Free<CfgParserKey*>());
    o_key_list.clear();

    // Handle parsed data.
    _texture_main[FOCUSED_STATE_FOCUSED] = th->getTexture(o_value_focused);
    _texture_main[FOCUSED_STATE_UNFOCUSED] = th->getTexture(o_value_unfocused);
    if (Util::splitString(o_value_pad, tok, " \t", 4) == 4) {
        for (uint i = 0; i < PAD_NO; ++i)
            _pad[i] = strtol (tok[i].c_str (), NULL, 10);
    }

    op_sub_2 = op_sub->find_section ("TAB");
    if (op_sub_2) {
        op_sub_2 = op_sub_2->get_section ();
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            op_value = op_sub_2->find_entry (_fs_map[FocusedState (i)]);
            if (op_value) {
                _texture_tab[i] = th->getTexture (op_value->get_value ());
            }
        }
    }

    op_sub_2 = op_sub->find_section ("SEPARATOR");
    if (op_sub_2) {
        op_sub_2 = op_sub_2->get_section ();

        o_key_list.push_back (new CfgParserKeyString ("FOCUSED", o_value_focused,
                              "Empty", th->getLengthMin ()));
        o_key_list.push_back (new CfgParserKeyString ("UNFOCUSED", o_value_unfocused,
                              "Empty", th->getLengthMin ()));

        // Parse data
        op_sub_2->parse_key_values (o_key_list.begin (), o_key_list.end ());

        // Free up resources
        for_each (o_key_list.begin (), o_key_list.end (),
                  Util::Free<CfgParserKey*>());
        o_key_list.clear();

        // Handle parsed data.
        _texture_separator[FOCUSED_STATE_FOCUSED] =
            th->getTexture(o_value_focused);
        _texture_separator[FOCUSED_STATE_UNFOCUSED] =
            th->getTexture(o_value_unfocused);
    }


    op_sub_2 = op_sub->find_section ("FONT");
    if (op_sub_2) {
        op_sub_2 = op_sub_2->get_section ();
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            op_value = op_sub_2->find_entry (_fs_map[FocusedState(i)]);
            if (op_value) {
                _font[i] = FontHandler::instance()->getFont(op_value->get_value ());
            }
        }
    } else {
        cerr << " *** WARNING: no font section in decor: " << _name << endl;
    }

    op_sub_2 = op_sub->find_section ("FONTCOLOR");
    if (op_sub_2) {
        op_sub_2 = op_sub_2->get_section ();
        for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
            op_value = op_sub_2->find_entry (_fs_map[FocusedState(i)]);
            if (op_value) {
                _font_color[i] = FontHandler::instance()->getColor(op_value->get_value ());
            }
        }
    }

    loadButtons (op_sub->find_section ("BUTTONS"));
    loadBorder (op_sub->find_section ("BORDER"));

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

        _texture_tab[i] = NULL;
        _font[i] = NULL;
        _font_color[i] = NULL;
    }

    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        th->returnTexture(_texture_main[i]);
        th->returnTexture(_texture_separator[i]);
        _texture_main[i] = NULL;
        _texture_separator[i] = NULL;

        for (uint j = 0; j < BORDER_NO_POS; ++j) {
            th->returnTexture(_texture_border[i][j]);
            _texture_border[i][j] = NULL;
        }
    }

    list<Theme::PDecorButtonData*>::iterator it(_button_list.begin());
    for (; it != _button_list.end(); ++it) {
        delete *it;
    }
    _button_list.clear();
}

//! @brief Checks data properties, prints warning and tries to fix.
void
Theme::PDecorData::check(void)
{
    // check values
    if (_title_width_max > 100) {
        cerr << " *** WARNING: " << _name << " WIDTHMAX > 100" << endl;
        _title_width_max = 100;
    }

    checkTextures();
    checkFonts();
    checkBorder();
    checkColors();
}

//! @brief Loads border data.
void
Theme::PDecorData::loadBorder (CfgParser::Entry *op_section)
{
    if (!op_section) {
        return;
    }
    op_section = op_section->get_section ();

    TextureHandler *th = TextureHandler::instance(); // convenience

    CfgParser::Entry *op_sub, *op_value;

    op_sub = op_section->find_section ("FOCUSED");
    if (op_sub) {
        op_sub = op_sub->get_section ();
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            op_value = op_sub->find_entry (_border_map[BorderPosition (i)]);
            if (op_value) {
                _texture_border[FOCUSED_STATE_FOCUSED][i] =
                    th->getTexture (op_value->get_value ());
            }
        }
    }

    op_sub = op_section->find_section ("UNFOCUSED");
    if (op_sub) {
        op_sub = op_sub->get_section ();
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            op_value = op_sub->find_entry (_border_map[BorderPosition (i)]);
            if (op_value) {
                _texture_border[FOCUSED_STATE_UNFOCUSED][i] =
                    th->getTexture (op_value->get_value ());
            }
        }
    }
}

//! @brief Loads button data.
void
Theme::PDecorData::loadButtons (CfgParser::Entry *op_section)
{
    if (!op_section) {
        return;
    }
    op_section = op_section->get_section ();

    while (op_section = op_section->get_section_next()) {
        Theme::PDecorButtonData *btn = new Theme::PDecorButtonData ();
        if (btn->load (op_section)) {
            _button_list.push_back(btn);
        } else {
            delete btn;
        }
    }
}

//! @brief Checks for NULL textures, prints warning and sets empty texture
void
Theme::PDecorData::checkTextures(void)
{
    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        if (!_texture_tab[i]) {
            cerr << " *** WARNING: " << _name << " missing tab texture state "
                 << _fs_map[FocusedState(i)] << endl;
            _texture_tab[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }
    for (uint i = 0; i < FOCUSED_STATE_FOCUSED_SELECTED; ++i) {
        if (!_texture_main[i]) {
            cerr << " *** WARNING: " << _name << " missing main texture state "
                 << _fs_map[FocusedState(i)] << endl;
            _texture_main[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (!_texture_separator[i]) {
            cerr << " *** WARNING: " << _name << " missing tab texture state "
                 << _fs_map[FocusedState(i)] << endl;
            _texture_separator[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }
}

//! @brief Checks for NULL fonts, prints warning and sets empty font
void
Theme::PDecorData::checkFonts(void)
{
    // the only font that's "obligatory" is the standard focused font,
    // others are only used if availible so we only check the focused font.
    if (!_font[FOCUSED_STATE_FOCUSED]) {
        cerr << " *** WARNING: " << _name << " missing font state "
             << _fs_map[FOCUSED_STATE_FOCUSED] << endl;
        _font[FOCUSED_STATE_FOCUSED] = FontHandler::instance()->getFont("");
    }
}

//! @brief Checks for NULL border PTextures.
void
Theme::PDecorData::checkBorder(void)
{
    for (uint state = FOCUSED_STATE_FOCUSED; state < FOCUSED_STATE_FOCUSED_SELECTED; ++state) {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            if (!_texture_border[state][i]) {
                cerr << " *** WARNING: " << _name << " missing border texture "
                     << _border_map[BorderPosition(i)] << " "
                     << _fs_map[FocusedState(state)] << endl;
                _texture_border[state][i] =
                    TextureHandler::instance()->getTexture("EMPTY");
            }
        }
    }
}

//! @brief Checks for NULL colors, prints warning and sets empty color
void
Theme::PDecorData::checkColors(void)
{
    for (uint i = 0; i < FOCUSED_STATE_NO; ++i) {
        if (!_font_color[i]) {
            cerr << " *** WARNING: " << _name << " missing font color state "
                 << _fs_map[FocusedState(i)] << endl;
            _font_color[i] = FontHandler::instance()->getColor("#000000");
        }
    }
}

// Theme::PMenuData

//! @brief PMenuData constructor
Theme::PMenuData::PMenuData(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        _font[i] = NULL;
        _color[i] = NULL;
        _tex_menu[i] = NULL;
        _tex_item[i] = NULL;
        _tex_arrow[i] = NULL;
    }
    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        _tex_sep[i] = NULL;
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

//! @brief Parses CfgParser::Entry op_section, loads and verifies data.
//! @param op_section CfgParser::Entry with pmenu configuration.
void
Theme::PMenuData::load(CfgParser::Entry *op_section)
{
    op_section = op_section->get_section ();

    CfgParser::Entry *op_value;
    op_value = op_section->find_entry ("PAD");
    if (op_value) {
        vector<string> tok;
        if (Util::splitString (op_value->get_value (), tok, " \t", 4) == 4) {
            for (int i = 0; i < PAD_NO; ++i) {
                _pad[i] = strtol (tok[i].c_str(), NULL, 10);
            }
        }
    }

    op_value = op_section->find_section ("FOCUSED");
    if (op_value) {
        loadState (op_value, OBJECT_STATE_FOCUSED);
    }
    
    op_value = op_section->find_section ("UNFOCUSED");
    if (op_value) {
        loadState (op_value, OBJECT_STATE_UNFOCUSED);
    }
    
    op_value = op_section->find_section ("SELECTED");
    if (op_value) {
        loadState (op_value, OBJECT_STATE_SELECTED);
    }

    check();
}

//! @brief Unloads data.
void
Theme::PMenuData::unload(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        FontHandler::instance()->returnFont(_font[i]);
        FontHandler::instance()->returnColor(_color[i]);
        TextureHandler::instance()->returnTexture(_tex_menu[i]);
        TextureHandler::instance()->returnTexture(_tex_item[i]);
        TextureHandler::instance()->returnTexture(_tex_arrow[i]);

        _font[i] = NULL;
        _color[i] = NULL;
        _tex_menu[i] = NULL;
        _tex_item[i] = NULL;
        _tex_arrow[i] = NULL;
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        TextureHandler::instance()->returnTexture(_tex_sep[i]);
        _tex_sep[i] = NULL;
    }
}

//! @brief Check data properties, prints warning and tries to fix.
void
Theme::PMenuData::check(void)
{
    for (uint i = 0; i <= OBJECT_STATE_NO; ++i) {
        if (!_font[i]) {
            _font[i] = FontHandler::instance()->getFont("");
        }
        if (!_color[i]) {
            _color[i] = FontHandler::instance()->getColor("#000000");
        }
        if (!_tex_menu[i]) {
            _tex_menu[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (!_tex_item[i]) {
            _tex_item[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
        if (!_tex_arrow[i]) {
            _tex_arrow[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }

    for (uint i = 0; i < OBJECT_STATE_NO; ++i) {
        if (!_tex_sep[i]) {
            _tex_sep[i] = TextureHandler::instance()->getTexture("EMPTY");
        }
    }
}

//! @brief
void
Theme::PMenuData::loadState (CfgParser::Entry *op_section, ObjectState state)
{
    op_section = op_section->get_section ();

    list<CfgParserKey*> o_key_list;
    string o_value_font, o_value_background, o_value_item;
    string o_value_text, o_value_arrow, o_value_separator;

    o_key_list.push_back (new CfgParserKeyString ("FONT", o_value_font));
    o_key_list.push_back (new CfgParserKeyString ("BACKGROUND", o_value_background,
                          "Solid #ffffff"));
    o_key_list.push_back (new CfgParserKeyString ("ITEM", o_value_item,
                          "Solid #ffffff"));
    o_key_list.push_back (new CfgParserKeyString ("TEXT", o_value_text,
                          "Solid #000000"));
    o_key_list.push_back (new CfgParserKeyString ("ARROW", o_value_arrow,
                          "Solid #000000"));
    if (state < OBJECT_STATE_SELECTED)
    {
        o_key_list.push_back (new CfgParserKeyString ("SEPARATOR",
                              o_value_separator,
                              "Solid #000000"));
    }

    op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

    for_each (o_key_list.begin (), o_key_list.end (),
              Util::Free<CfgParserKey*>());

    TextureHandler *th = TextureHandler::instance ();

    // Handle parsed data.
    _font[state] = FontHandler::instance ()->getFont (o_value_font);
    _tex_menu[state] = th->getTexture (o_value_background);
    _tex_item[state] = th->getTexture (o_value_item);
    _color[state] = FontHandler::instance ()->getColor (o_value_text);
    _tex_arrow[state] = th->getTexture (o_value_arrow);
    if (state < OBJECT_STATE_SELECTED) {
        _tex_sep[state] = th->getTexture(o_value_separator);
    }
}

// Theme::TextDialogData

//! @brief TextDialogData constructor.
Theme::TextDialogData::TextDialogData(void) :
        _font(NULL), _color(NULL), _tex(NULL)
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

//! @brief Parses CfgParser::Entry op_section, loads and verifies data.
//! @param op_section CfgParser::Entry with textdialog configuration.
void
Theme::TextDialogData::load (CfgParser::Entry *op_section)
{
    op_section = op_section->get_section ();

    list<CfgParserKey*> o_key_list;
    string o_value_font, o_value_text, o_value_texture, o_value_pad;

    o_key_list.push_back (new CfgParserKeyString ("FONT", o_value_font));
    o_key_list.push_back (new CfgParserKeyString ("TEXT", o_value_text,
                          "#000000"));
    o_key_list.push_back (new CfgParserKeyString ("TEXTURE", o_value_texture,
                          "Solid #ffffff"));
    o_key_list.push_back (new CfgParserKeyString ("PAD", o_value_pad,
                          "0 0 0 0", 7));

    op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

    for_each (o_key_list.begin (), o_key_list.end (),
              Util::Free<CfgParserKey*>());

    // Handle parsed data.
    _font = FontHandler::instance ()->getFont (o_value_font);
    _color = FontHandler::instance ()->getColor (o_value_text);
    _tex = TextureHandler::instance ()->getTexture (o_value_texture);

    vector<string> tok;
    if (Util::splitString (o_value_pad, tok, " \t", 4) == 4) {
        for (uint i = 0; i < PAD_NO; ++i) {
            _pad[i] = strtol(tok[i].c_str(), NULL, 10);
        }
    }

    check();
}

//! @brief Unloads data.
void
Theme::TextDialogData::unload(void)
{
    FontHandler::instance()->returnFont(_font);
    FontHandler::instance()->returnColor(_color);
    TextureHandler::instance()->returnTexture(_tex);

    _font = NULL;
    _tex = NULL;
    _color = NULL;
}

//! @brief Check data properties, prints warning and tries to fix.
//! @todo print warnings
void
Theme::TextDialogData::check(void)
{
    if (!_font) {
        _font = FontHandler::instance()->getFont("");
    }
    if (!_color) {
        _color = FontHandler::instance()->getColor("#000000");
    }
    if (!_tex) {
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
void
Theme::WorkspaceIndicatorData::load(CfgParser::Entry *cs)
{
    cs = cs->get_section();

    list<CfgParserKey*> key_list;

    string value_font, value_color, value_tex_bg;
    string value_tex_ws, value_tex_ws_act;

    key_list.push_back(new CfgParserKeyString("FONT", value_font));
    key_list.push_back(new CfgParserKeyString("COLOR", value_color));
    key_list.push_back(new CfgParserKeyString("BACKGROUND", value_tex_bg));
    key_list.push_back(new CfgParserKeyString("WORKSPACE", value_tex_ws));
    key_list.push_back(new CfgParserKeyString("WORKSPACEACTIVE", value_tex_ws_act));
    key_list.push_back(new CfgParserKeyInt("EDGEPADDING", edge_padding, 5, 0));
    key_list.push_back(new CfgParserKeyInt("WORKSPACEPADDING", workspace_padding, 2, 0));

    cs->parse_key_values(key_list.begin(), key_list.end());
    for_each (key_list.begin (), key_list.end (), Util::Free<CfgParserKey*>());  

    font = FontHandler::instance()->getFont(value_font);
    font_color = FontHandler::instance()->getColor(value_color);
    texture_background = TextureHandler::instance()->getTexture(value_tex_bg);
    texture_workspace = TextureHandler::instance()->getTexture(value_tex_ws);
    texture_workspace_act = TextureHandler::instance()->getTexture(value_tex_ws_act);

    check();
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

// Theme

//! @brief Theme constructor
Theme::Theme(PScreen *scr) :
        _scr(scr), _image_handler(NULL),
        _is_loaded(false), _invert_gc(None)
{
    // image handler
    _image_handler = new ImageHandler();

    // window gc's
    XGCValues gv;
    gv.function = GXinvert;
    gv.subwindow_mode = IncludeInferiors;
    gv.line_width = 1;
    _invert_gc = XCreateGC(_scr->getDpy(), _scr->getRoot(),
                           GCFunction|GCSubwindowMode|GCLineWidth, &gv);


    _scr->grabServer();

    load(Config::instance()->getThemeFile());

    _scr->ungrabServer(true);
}

//! @brief Theme destructor
Theme::~Theme(void)
{
    unload(); // should clean things up

    XFreeGC(_scr->getDpy(), _invert_gc);

    delete _image_handler;
}

//! @brief Loads the "ThemeFile", unloads any previous loaded theme.
void
Theme::load(const std::string &dir)
{
    if (_is_loaded) {
        unload();
    }
    
    CfgParser theme;

    _theme_dir = dir;
    if (_theme_dir.size ()) {
        if (_theme_dir.at (_theme_dir.size() - 1) != '/') {
            _theme_dir.append("/");
        }
    } else {
        cerr << " *** WARNING: empty theme directory name, using default." << endl;
        _theme_dir = DATADIR "/pekwm/themes/default/";
    }

    string theme_file = _theme_dir + string("theme");
    if (!theme.parse (theme_file, CfgParserSource::SOURCE_FILE)) {
        _theme_dir = DATADIR "/pekwm/themes/default/";
        theme_file = _theme_dir + string ("theme");
        if (!theme.parse (theme_file, CfgParserSource::SOURCE_FILE)) {
            cerr << " *** WARNING: couldn't load " << _theme_dir
                 << " or default theme." << endl;
        }
    }

    CfgParser::Entry *op_section, *op_value;

    // Set image basedir.
    _image_handler->setDir(_theme_dir);

    // Load decor data.
    op_section = theme.get_entry_root ()->find_section ("PDECOR");
    if (op_section) {
        // Get section data, don't need name or value.
        op_section = op_section->get_section ();

        //op_section = op_section->get_section ();
        while (op_section = op_section->get_section_next ()) {
            Theme::PDecorData *data = new Theme::PDecorData();
            if (data->load (op_section)) {
                _pdecordata_map[data->getName()] = data;
            } else {
                delete data;
            }
        }
    }

    if (!getPDecorData ("DEFAULT")) {
        cerr << " *** WARNING: theme doesn't contain any DEFAULT decor." << endl;

        // Create DEFAULT decor, let check fill it up with empty but non-null data.
        Theme::PDecorData *data = new Theme::PDecorData();
        data->setName("DEFAULT");
        data->check();
        _pdecordata_map[data->getName()] = data;
    }

    // Load menu data.
    op_section = theme.get_entry_root ()->find_section ("MENU");
    if (op_section) {
        _menu_data.load(op_section);
    } else {
        cerr << " *** WARNING: missing menu section" << endl;
        _menu_data.check ();
    }

    // Load StatusWindow data.
    op_section = theme.get_entry_root ()->find_section ("STATUS");
    if (op_section) {
        _status_data.load (op_section);
    } else {
        cerr << " *** WARNING: missing status section" << endl;
        _status_data.check ();
    }

    // Load CmdDialog data.
    op_section = theme.get_entry_root ()->find_section ("CMDDIALOG");
    if (op_section) {
        _cmd_d_data.load (op_section);
    } else {
        cerr << " *** WARNING: missing cmddialog section" << endl;
        _cmd_d_data.check ();
    }

    // Load WorkspaceIndicator data.
    op_section = theme.get_entry_root()->find_section("WORKSPACEINDICATOR");
    if (op_section) {
      _workspace_indicator_data.load(op_section);
    } else {
      cerr << " *** WARNING: missing workspaceindicator section" << endl;
      _workspace_indicator_data.check();
    }

#ifdef HARBOUR
    // Load Harbour texture.
    op_section = theme.get_entry_root ()->find_section ("HARBOUR");
    if (op_section) {
        op_section = op_section->get_section ();

        op_value = op_section->find_entry ("TEXTURE");
        if (op_value) {
            _harbour_texture = TextureHandler::instance ()->getTexture (op_value->get_value ());
        }
    } else {
        cerr << " *** WARNING: missing harbour section" << endl;
    }
    
    if (!_harbour_texture) {
        cerr << " *** WARNING: missing harbour texture" << endl;
        _harbour_texture = TextureHandler::instance ()->getTexture("EMPTY");
    }
#endif // HARBOUR

    _is_loaded = true;
}

//! @brief Unloads all pixmaps, fonts, gc and colors allocated by the theme.
void
Theme::unload(void) {
    map<string, Theme::PDecorData*>::iterator it(_pdecordata_map.begin());
    for (; it != _pdecordata_map.end(); ++it) {
        delete it->second;
    }
    _pdecordata_map.clear();

#ifdef HARBOUR
    TextureHandler::instance()->returnTexture(_harbour_texture);
    _harbour_texture = NULL;
#endif // HARBOUR

    // unload menu
    _menu_data.unload();

    _status_data.unload();
    _cmd_d_data.unload();

    _is_loaded = false;
}
