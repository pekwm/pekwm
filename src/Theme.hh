//
// Theme.hh for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _THEME_HH_
#define _THEME_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include "CfgParser.hh"
#include "Action.hh" // ActionEvent
#include "PFont.hh" // PFont::Color
#include "ParseUtil.hh"

class PScreen;
class PTexture;
class Button;
class ButtonData;
class ImageHandler;

#include <string>
#include <map>

//! @brief Theme data parser and container.
class Theme
{
public:
    //! @brief Theme data parser and container for PDecor::Button
    class PDecorButtonData {
    public:
        PDecorButtonData(void);
        ~PDecorButtonData(void);

        //! @brief Returns wheter the button is positioned relative to the left title edge.
        inline bool isLeft(void) const { return _left; }
        //! @brief Returns width of button.
        inline uint getWidth(void) const { return _width; }
        //! @brief Returns height of button.
        inline uint getHeight(void) const { return _height; }

        //! @brief Returns PTexture used in ButtonState state.
        inline PTexture *getTexture(ButtonState state) {
            return _texture[(state != BUTTON_STATE_NO) ? state : 0];
        }
        //! @brief Returns iterator to the first ActionEvent.
        inline std::list<ActionEvent>::iterator begin(void) {
            return _ae_list.begin();
        }
        //! @brief Return iterator to the last+1 ActionEvent.
        inline std::list<ActionEvent>::iterator end(void) {
            return _ae_list.end();
        }

        bool load(CfgParser::Entry *cs);
        void unload(void);

    private:
        void check(void);

    private:
        std::list<ActionEvent> _ae_list;
        PTexture *_texture[BUTTON_STATE_NO];

        bool _left;
        uint _width, _height;
    };

    //! @brief PDecor theme data container and parser.
    class PDecorData {
    public:
        PDecorData(void);
        ~PDecorData(void);

        //! @brief Returns decor name.
        inline const std::string &getName(void) const { return _name; }
        //! @brief Sets decor name.
        inline void setName(const std::string &name) { _name = name; }

        //! @brief Returns title height.
        inline int getTitleHeight(void) const { return _title_height; }
        //! @brief Returns title minimum width (0 for full width title).
        inline int getTitleWidthMin(void) const { return _title_width_min; }
        //! @brief Returns title maximum width in procent.
        inline int getTitleWidthMax(void) const { return _title_width_max; }
        //! @brief Returns title text pad for dir.
        inline int getPad(PadType pad) const {
            return _pad[(pad != PAD_NO) ? pad : 0];
        }

        //! @brief Returns wheter all items in the title have same width.
        inline bool isTitleWidthSymetric(void) const {
            return _title_width_symetric;
        }
      //! @brief Returns wheter titlebar height should be relative the font height
      inline bool isTitleHeightAdapt(void) const { return _title_height_adapt; }

        // Title textures

        //! @brief Returns background PTexture used in FocusedState state.
        inline PTexture *getTextureMain(FocusedState state) {
            return _texture_main[(state < FOCUSED_STATE_FOCUSED_SELECTED)
                                 ? state : 0];
        }
        //! @brief Returns tab PTexture used in FocusedState state.
        inline PTexture *getTextureTab(FocusedState state) {
            return _texture_tab[(state != FOCUSED_STATE_NO) ? state : 0];
        }
        //! @brief Returns separator PTexture used in FocusedState state.
        inline PTexture *getTextureSeparator(FocusedState state) {
            return _texture_separator[(state < FOCUSED_STATE_FOCUSED_SELECTED)
                                      ? state : 0];
        }

        // font

        //! @brief Returns PFont used in FocusedState state.
        inline PFont *getFont(FocusedState state) const {
            return _font[((state == FOCUSED_STATE_NO) || (_font[state] == NULL))
                         ? FOCUSED_STATE_FOCUSED : state];
        }
        //! @brief Return PFont::Color used in FocusedState state.
        inline PFont::Color *getFontColor(FocusedState state) {
            return _font_color[(state != FOCUSED_STATE_NO) ? state : 0];
        }

        // border

        //! @brief Return border PTexture used in FocusedState state for pos.
        inline PTexture *getBorderTexture(FocusedState state, BorderPosition pos) {
            return _texture_border[(state < FOCUSED_STATE_FOCUSED_SELECTED)
                                   ? state : 0][pos];
        }

        // button

        //! @brief Return iterator to the first Theme::PDecorButtonData.
        inline std::list<Theme::PDecorButtonData*>::iterator buttonBegin(void) {
            return _button_list.begin();
        }
        //! @brief Return iterator to the last+1 Theme::PDecorButtonData.
        inline std::list<Theme::PDecorButtonData*>::iterator buttonEnd(void) {
            return _button_list.end();
        }

        bool load(CfgParser::Entry *cs);
        void unload(void);

        // sanity functions
        void check(void);

    private:
        void loadBorder(CfgParser::Entry *cs);
        void loadButtons(CfgParser::Entry *cs);

        void checkTextures(void);
        void checkFonts(void);
        void checkBorder(void);
        void checkColors(void);

    private:
        std::string _name;

        // size, padding etc
        int _title_height;
        int _title_width_min, _title_width_max;
        int _pad[PAD_NO];
        bool _title_width_symetric;
        bool _title_height_adapt;

        // title
        PTexture *_texture_main[FOCUSED_STATE_FOCUSED_SELECTED];
        PTexture *_texture_tab[FOCUSED_STATE_NO];
        PTexture *_texture_separator[FOCUSED_STATE_FOCUSED_SELECTED];

        // font
        PFont *_font[FOCUSED_STATE_NO];
        PFont::Color *_font_color[FOCUSED_STATE_NO];

        // border
        PTexture *_texture_border[FOCUSED_STATE_FOCUSED_SELECTED][BORDER_NO_POS];

        // buttons
        std::list<Theme::PDecorButtonData*> _button_list;

        static std::map<FocusedState, std::string> _fs_map;
        static std::map<BorderPosition, std::string> _border_map;
    };

    //! @brief PMenu theme data container and parser.
    class PMenuData {
    public:
        PMenuData(void);
        ~PMenuData(void);

        //! @brief Returns PFont used in ObjectState state.
        inline PFont *getFont(ObjectState state) { return _font[state]; }
        //! @brief Returns PFont::Color used in ObjectState state.
        inline PFont::Color *getColor(ObjectState state) { return _color[state]; }
        //! @brief Returns menu PTexture used in ObjectState state.
        inline PTexture *getTextureMenu(ObjectState state) {
            return _tex_menu[state];
        }
        //! @brief Returns item PTexture used in ObjectState state.
        inline PTexture *getTextureItem(ObjectState state) {
            return _tex_item[state];
        }
        //! @brief Returns arrow PTexture used in ObjectState state.
        inline PTexture *getTextureArrow(ObjectState state) {
            return _tex_arrow[state];
        }
        //! @brief Returns separator PTexture used in ObjectState state.
        inline PTexture *getTextureSeparator(ObjectState state) {
            return _tex_sep[(state < OBJECT_STATE_SELECTED)
                            ? state : OBJECT_STATE_FOCUSED];
        }
        //! @brief Returns text pad in PadType dir.
        inline uint getPad(PadType dir) const {
            return _pad[(dir != PAD_NO) ? dir : 0];
        }

        void load(CfgParser::Entry *cs);
        void unload(void);

        // sanity functions
        void check(void);

    private:
        void loadState(CfgParser::Entry *cs, ObjectState state);

    private:
        PFont *_font[OBJECT_STATE_NO + 1];
        PFont::Color *_color[OBJECT_STATE_NO + 1];
        PTexture *_tex_menu[OBJECT_STATE_NO + 1];
        PTexture *_tex_item[OBJECT_STATE_NO + 1];
        PTexture *_tex_arrow[OBJECT_STATE_NO + 1];
        PTexture *_tex_sep[OBJECT_STATE_NO];

        uint _pad[PAD_NO];
    };

    //! @brief CmdDialog/StatusWindow theme data container and parser.
    class TextDialogData {
    public:
        TextDialogData(void);
        ~TextDialogData(void);

        //! @brief Returns PFont.
        inline PFont *getFont(void) { return _font; }
        //! @brief Returns PFont::Color.
        inline PFont::Color *getColor(void) { return _color; }
        //! @brief Returns background texture.
        inline PTexture *getTexture(void) { return _tex; }
        //! @brief Returns text pad in PadType dir.
        inline uint getPad(PadType dir) const {
            return _pad[(dir != PAD_NO) ? dir : 0];
        }

        void load(CfgParser::Entry *cs);
        void unload(void);

        // sanity functions
        void check(void);

    private:
        PFont *_font;
        PFont::Color *_color;
        PTexture *_tex;

        uint _pad[PAD_NO];
    };

  /**
   * Class holding WorkspaceIndicator theme data.
   */
  class WorkspaceIndicatorData {
  public:
    WorkspaceIndicatorData(void);
    ~WorkspaceIndicatorData(void);

    void load(CfgParser::Entry *cs);
    void unload(void);

    void check(void);

  public:
    PFont *font;
    PFont::Color *font_color;
    PTexture *texture_background;
    PTexture *texture_workspace;
    PTexture *texture_workspace_act;

    int edge_padding;
    int workspace_padding;
  };

    Theme(PScreen *scr);
    ~Theme(void);

    void load(const std::string &dir);
    void unload(void);

    inline const GC &getInvertGC(void) const { return _invert_gc; }

    inline std::map<std::string, Theme::PDecorData*>::const_iterator decor_begin(void) { return _pdecordata_map.begin(); }
    inline std::map<std::string, Theme::PDecorData*>::const_iterator decor_end(void) { return _pdecordata_map.end(); }

    Theme::PDecorData *getPDecorData(const std::string &name) {
        std::map<std::string, Theme::PDecorData*>::iterator it = _pdecordata_map.begin();
        for (; it != _pdecordata_map.end(); ++it) {
            if (strcasecmp(it->first.c_str(), name.c_str()) == 0) {
                return it->second;
            }
        }
        return NULL;
    }

  Theme::WorkspaceIndicatorData &getWorkspaceIndicatorData(void) { return _workspace_indicator_data; }

    // menu
    inline Theme::PMenuData *getMenuData(void) { return &_menu_data; }

#ifdef HARBOUR
    inline PTexture *getHarbourTexture(void) const { return _harbour_texture; }
#endif // HARBOUR

    // status/cmd
    inline Theme::TextDialogData *getStatusData(void) { return &_status_data; }
    inline Theme::TextDialogData *getCmdDialogData(void) { return &_cmd_d_data; }

private:
    PScreen *_scr;
    ImageHandler *_image_handler;
    std::string _theme_dir;

    bool _is_loaded;

    // gc
    GC _invert_gc;

    // frame decors
    std::map<std::string, Theme::PDecorData*> _pdecordata_map;

    // menu
    Theme::PMenuData _menu_data;

    // harbour
#ifdef HARBOUR
    PTexture *_harbour_texture;
#endif // HARBOUR

    // status window
    TextDialogData _status_data, _cmd_d_data;

  WorkspaceIndicatorData _workspace_indicator_data;
};

#endif // _THEME_HH_
