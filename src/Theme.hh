

//
// Theme.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _THEME_HH_
#define _THEME_HH_

#include "config.h"

#include "pekwm.hh"

#include "CfgParser.hh"
#include "Action.hh" // ActionEvent
#include "PFont.hh" // PFont::Color
#include "ParseUtil.hh"

class PTexture;
class Button;
class ButtonData;
class FontHandler;
class ImageHandler;
class TextureHandler;

#include <string>
#include <map>
#include <vector>

//! @brief Theme data parser and container.
class Theme
{
public:
    //! @brief Theme data parser and container for PDecor::Button
    class PDecorButtonData {
    public:
        PDecorButtonData(TextureHandler* th);
        ~PDecorButtonData(void);

        //! @brief Returns whether the shape (derived from the alpha channel) should be set.
        inline bool setShape(void) const { return _shape; }
        //! @brief Returns whether the button is positioned relative to the left title edge.
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
        std::vector<ActionEvent>::iterator begin(void) {
            return _aes.begin();
        }
        //! @brief Return iterator to the last+1 ActionEvent.
        std::vector<ActionEvent>::iterator end(void) {
            return _aes.end();
        }

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);

    private:
        TextureHandler* _th;
        bool _loaded;

        std::vector<ActionEvent> _aes;
        PTexture *_texture[BUTTON_STATE_NO];

        bool _shape:1, _left:1;
        uint _width, _height;
    };

    //! @brief PDecor theme data container and parser.
    class PDecorData {
    public:
        PDecorData(FontHandler* fh, TextureHandler* th, const char *name=0);
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
            return _font[((state == FOCUSED_STATE_NO) || ! _font[state])
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
        std::vector<Theme::PDecorButtonData*>::const_iterator buttonBegin(void) {
            return _buttons.begin();
        }
        //! @brief Return iterator to the last+1 Theme::PDecorButtonData.
        std::vector<Theme::PDecorButtonData*>::const_iterator buttonEnd(void) {
            return _buttons.end();
        }

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);

    private:
        void loadBorder(CfgParser::Entry *cs);
        void loadButtons(CfgParser::Entry *cs);

        void checkTextures(void);
        void checkFonts(void);
        void checkBorder(void);
        void checkColors(void);

    private:
        FontHandler* _fh;
        TextureHandler* _th;

        bool _loaded;
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

        std::vector<Theme::PDecorButtonData*> _buttons;

        static std::map<FocusedState, std::string> _fs_map;
        static std::map<BorderPosition, std::string> _border_map;
    };

    //! @brief PMenu theme data container and parser.
    class PMenuData {
    public:
        PMenuData(FontHandler* fh, TextureHandler* th);
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

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);

    private:
        void loadState(CfgParser::Entry *cs, ObjectState state);

    private:
        FontHandler* _fh;
        TextureHandler* _th;
        bool _loaded;

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
        TextDialogData(FontHandler* fh, TextureHandler* th);
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

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);

    private:
        FontHandler* _fh;
        TextureHandler* _th;
        bool _loaded;

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
        WorkspaceIndicatorData(FontHandler* fh, TextureHandler *th);
        ~WorkspaceIndicatorData(void);

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);

    private:
        FontHandler* _fh;
        TextureHandler* _th;
        bool _loaded;

    public:
        PFont *font;
        PFont::Color *font_color;
        PTexture *texture_background;
        PTexture *texture_workspace;
        PTexture *texture_workspace_act;

        int edge_padding;
        int workspace_padding;
    };

    /**
      * Class holding harbour theme data.
      */
    class HarbourData {
    public:
        HarbourData(TextureHandler* th);
        ~HarbourData(void);

        inline PTexture *getTexture(void) const { return _texture; }

        bool load(CfgParser::Entry *section);
        void unload(void);
        void check(void);
    private:
        TextureHandler* _th;

        bool _loaded;
        /**< Texture for rendering dockapps in the harbour. */
        PTexture *_texture;
    };

    inline Theme::HarbourData *getHarbourData(void) { return &_harbour_data; }

    Theme(FontHandler *fh, ImageHandler *ih, TextureHandler *th);
    ~Theme(void);

    bool load(const std::string &dir);
    void unload(void);

    inline const GC &getInvertGC(void) const { return _invert_gc; }

    std::map<std::string, Theme::PDecorData*>::const_iterator decor_begin(void) {
        return _pdecordata_map.begin();
    }
    std::map<std::string, Theme::PDecorData*>::const_iterator decor_end(void) {
        return _pdecordata_map.end();
    }

    /**
     * Find PDecorData based on name.
     */
    Theme::PDecorData *getPDecorData(const std::string &name) {
        for (auto it : _pdecordata_map) {
            if (strcasecmp(it.first.c_str(), name.c_str()) == 0) {
                return it.second;
            }
        }

        // Backwards compatibility, CMDDIALOG was used instead of
        // INPUTDIALOG previously.
        if (strcasecmp("INPUTDIALOG", name.c_str()) == 0) {
            return getPDecorData("CMDDIALOG");
        }

        return 0;
    }

    Theme::WorkspaceIndicatorData &getWorkspaceIndicatorData(void) {
        return _ws_indicator_data;
    }

    // menu
    inline Theme::PMenuData *getMenuData(void) { return &_menu_data; }

    // status/cmd
    inline Theme::TextDialogData *getStatusData(void) { return &_status_data; }
    inline Theme::TextDialogData *getCmdDialogData(void) { return &_cmd_d_data; }

private:
    void loadThemeRequire(CfgParser &theme_cfg, std::string &file);

private:
    FontHandler* _fh;
    ImageHandler* _ih;
    TextureHandler* _th;

    std::string _theme_dir; /**< Path to theme directory. */
    TimeFiles _cfg_files;

    bool _loaded;

    // gc
    GC _invert_gc;

    // frame decors
    std::map<std::string, Theme::PDecorData*> _pdecordata_map;

    // menu
    PMenuData _menu_data;
    /** Data for styling harbour. */
    HarbourData _harbour_data;

    // status window
    TextDialogData _status_data;
    TextDialogData _cmd_d_data;
    WorkspaceIndicatorData _ws_indicator_data;
};

namespace pekwm
{
    Theme* theme();
};

#endif // _THEME_HH_
