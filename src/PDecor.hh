//
// PDecor.hh for pekwm
// Copyright © 2004-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PDECOR_HH_
#define _PDECOR_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Config.hh"
#include "PWinObj.hh"
#include "Theme.hh" // for Theme::FrameData inlines
#include "PTexture.hh" // for border/image inlines

class ActionEvent;
class PFont;
class PWinObj;

/**
 * Create window attributes.
 */
class CreateWindowParams {
public:
    int depth;
    long mask;
    XSetWindowAttributes attr;
    Visual *visual;
};

//! @brief PWinObj container class with fancy decor.
class PDecor : public PWinObj
{
public:
    //! @brief Decor title button class.
    class Button : public PWinObj {
    public:
         Button(PWinObj *parent, Theme::PDecorButtonData *data, uint width, uint height);
        ~Button(void);

        ActionEvent *findAction(XButtonEvent *ev);
        ButtonState getState(void) const { return _state; }
        void setState(ButtonState state);

        //! @brief Returns wheter the button is positioned relative the left title edge.
        inline bool isLeft(void) const { return _left; }

    private:
        Theme::PDecorButtonData *_data;
        ButtonState _state;

        Pixmap _bg;
        bool _left;
    };

    //! @brief Decor title item class.
    class TitleItem {
    public:
        //! Info bitmask enum.
        enum Info {
            INFO_MARKED = (1 << 1),
            INFO_ID = (1 << 2)
        };

        TitleItem(void) : _count(0), _id(0), _info(0), _width(0) { }

        inline const std::wstring &getVisible(void) const { return _visible; }
        inline const std::wstring &getReal(void) const { return _real; }
        inline const std::wstring &getCustom(void) const { return _custom; }
        inline const std::wstring &getUser(void) const { return _user; }
 
        inline uint getCount(void) const { return _count; }
        inline uint getId(void) const { return _id; }
        inline bool isUserSet(void) const { return (_user.size() > 0); }
        inline bool isCustom(void) const { return (_custom.size() > 0); }
        inline uint getWidth(void) const { return _width; }

        inline void infoAdd(enum Info info) { _info |= info; }
        inline void infoRemove(enum Info info) { _info &= ~info; }
        inline bool infoIs(enum Info info) { return (_info&info); }

        void setReal(const std::wstring &real) {
            _real = real;
            if (! isUserSet() && ! isCustom()) {
                updateVisible();
            }
        }
        void setCustom(const std::wstring &custom) {
            _custom = custom;
            if (_custom.size() > 0 && ! isUserSet()) {
                updateVisible();
            }
        }
        void setUser(const std::wstring &user) {
            _user = user;
            updateVisible();
        }
        void setCount(uint count) { _count = count; }
        void setId(uint id) { _id = id; }
        inline void setWidth(uint width) { _width = width; }

        void updateVisible(void);

    private:
        std::wstring _visible; //!< Visible version of title
      
        std::wstring _real; //!< Title from client
        std::wstring _custom; //!< Custom (title rule) set version of title
        std::wstring _user; //!< User set version of title

        uint _count; //!< Number of title
        uint _id; //!< ID of title
        uint _info; //!< Info bitmask for extra title info

        uint _width;
    };

    PDecor(Theme *theme,
           const std::string decor_name = DEFAULT_DECOR_NAME,
           const Window child_window = None);
    virtual ~PDecor(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void mapWindowRaised(void);
    virtual void unmapWindow(void);

    virtual void move(int x, int y);
    virtual void resize(uint width, uint height);
    virtual void moveResize(int x, int y, uint width, uint height);
    virtual void raise(void);
    virtual void lower(void);

    virtual void setFocused(bool focused);
    virtual void setWorkspace(uint workspace);

    virtual void giveInputFocus(void);

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
    virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
    virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

    virtual bool operator == (const Window &window) {
        if ((_window == window) || (_title_wo == window) ||
                findButton(window) ||
                (getBorderPosition(window) != BORDER_NO_POS) ||
                ((_child) ? (*_child == window) : false)) {
            return true;
        }
        return false;
    }
    virtual bool operator != (const Window &window) {
        return !(*this == window);
    }
    // END - PWinObj interface.

    // START - PDecor interface.
    virtual bool allowMove(void) const { return true; }

    virtual void addChild(PWinObj *child, vector<PWinObj*>::iterator *it = 0);
    virtual void removeChild(PWinObj *child, bool do_delete = true);
    virtual void activateChild(PWinObj *child);

    virtual void updatedChildOrder(void) { }
    virtual void updatedActiveChild(void) { }

    virtual void getDecorInfo(wchar_t *buf, uint size);

    virtual void setShaded(StateAction sa);
    virtual void setSkip(uint skip);
    // END - PDecor interface.

    static vector<PDecor*>::const_iterator pdecor_begin(void) { return _pdecors.begin(); }
    static vector<PDecor*>::const_iterator pdecor_end(void) { return _pdecors.end(); }

    inline bool isSkip(uint skip) const { return (_skip&skip); }

    void addDecor(PDecor *decor);

    bool setDecor(const std::string &name);
    void setDecorOverride(StateAction sa, const std::string &name);
    void loadDecor(void);

    //! @brief Returns title Window.
    inline Window getTitleWindow(void) const { return _title_wo.getWindow(); }
    PDecor::Button *findButton(Window win);

    //! @brief Returns height of PDecor ignoring shaded state.
    inline uint getRealHeight(void) const {
        return (_shaded ? _real_height : _gm.height);
    }

    //! @brief Returns last click x root position.
    inline int getPointerX(void) const { return _pointer_x; }
    //! @brief Returns last click y root position.
    inline int getPointerY(void) const { return _pointer_y; }
    //! @brief Returns last click x window position.
    inline int getClickX(void) const { return _click_x; }
    //! @brief Returns last click y window position.
    inline int getClickY(void) const { return _click_y; }

    //! @brief Returns width of child container.
    inline uint getChildWidth(void) const {
        if ((borderLeft() + borderRight()) >= _gm.width) {
            return 1;
        }
        return (_gm.width - borderLeft() - borderRight());
    }
    //! @brief Returns height of child container.
    inline uint getChildHeight(void) const {
        if ((borderTop() + borderBottom() + getTitleHeight()) >= getRealHeight()) {
            return 1;
        }
        return (getRealHeight() - borderTop() - borderBottom() - getTitleHeight());
    }

    // child list actions

    //! @brief Returns number of children in PDecor.
    inline uint size(void) const { return _children.size(); }
    //! @brief Returns iterator to the first child in PDecor.
    inline vector<PWinObj*>::const_iterator begin(void) { return _children.begin(); }
    //! @brief Returns iterator to the last+1 child in PDecor.
    inline vector<PWinObj*>::const_iterator end(void) { return _children.end(); }

    //! @brief Returns pointer to active PWinObj.
    inline PWinObj *getActiveChild(void) { return _child; }
    PWinObj *getChildFromPos(int x);

    void activateChildNum(uint num);
    void activateChildRel(int off); // +/- relative from active
    void moveChildRel(int off); // +/- relative from active

    // title

    //! @brief Adds TitleItem to title list.
    inline void titleAdd(PDecor::TitleItem *ct) { _titles.push_back(ct); }
    //! @brief Removes all TitleItems from title list.
    inline void titleClear(void) { _titles.clear(); }
    //! @brief Sets active TitleItem.
    void titleSetActive(uint num) {
        _title_active = (num > _titles.size()) ? 0 : num;
    }

    // move and resize relative to the child instead of decor
    void moveChild(int x, int y);
    void resizeChild(uint width, uint height);

    // decor state

    //! @brief Returns wheter we have a border or not.
    inline bool hasBorder(void) const { return _border; }
    //! @brief Returns wheter we have a titlebar or not.
    inline bool hasTitlebar(void) const { return _titlebar; }
    //! @brief Returns wheter we are shaded or not.
    inline bool isShaded(void) const { return _shaded; }
    void setBorder(StateAction sa);
    void setTitlebar(StateAction sa);

    // decor element sizes
    uint getTitleHeight(void) const;

    // common actions like doMove
    void doMove(int x_root, int y_root);
    void doKeyboardMoveResize(void);

    //! @brief Returns border position Window win is at.
    inline BorderPosition getBorderPosition(Window win) const {
        for (uint i = 0; i < BORDER_NO_POS; ++i) {
            if (_border_win[i] == win) {
                return BorderPosition(i);
            }
        }
        return BORDER_NO_POS;

    }
    inline uint borderTop(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_TOP)->getHeight() : 0);
    }
    inline uint borderTopLeft(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_TOP_LEFT)->getWidth() : 0);
    }
    inline uint borderTopLeftHeight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_TOP_LEFT)->getHeight() : 0);
    }
    inline uint borderTopRight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_TOP_RIGHT)->getWidth() : 0);
    }
    inline uint	borderTopRightHeight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_TOP_RIGHT)->getHeight() : 0);
    }
    inline uint	borderBottom(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_BOTTOM)->getHeight() : 0);
    }
    inline uint borderBottomLeft(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_BOTTOM_LEFT)->getWidth() : 0);
    }
    inline uint borderBottomLeftHeight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_BOTTOM_LEFT)->getHeight() : 0);
    }
    inline uint borderBottomRight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_BOTTOM_RIGHT)->getWidth() : 0);
    }
    inline uint borderBottomRightHeight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_BOTTOM_RIGHT)->getHeight() : 0);
    }
    inline uint borderLeft(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_LEFT)->getWidth() : 0);
    }
    inline uint borderRight(void) const {
        return (_border ? _data->getBorderTexture(getFocusedState(false), BORDER_RIGHT)->getWidth() : 0);
    }

    void deiconify(void);
protected:
    // START - PDecor interface.
    virtual void renderTitle(void);
    virtual void renderButtons(void);
    virtual void renderBorder(void);
    virtual void setBorderShape(void); // shapes border corners
    virtual void applyBorderShape(void); // applies shape onto window

    virtual void loadTheme(void) { } // called after loadDecor, render child

    virtual int resizeHorzStep(int diff) const { return diff; }
    virtual int resizeVertStep(int diff) const { return diff; }
    // END - PDecor interface.

    void resizeTitle(void);

    //! @brief Returns font used at FocusedState state.
    inline PFont *getFont(FocusedState state) const { return _data->getFont(state); }

    uint getNearestHead(void);

    void checkSnap(void);
    void checkWOSnap(void);
    void checkEdgeSnap(void);

    bool setShape(void);
    void alignChild(PWinObj *child);
    void drawOutline(const Geometry &gm);

    inline FocusedState getFocusedState(bool selected) const {
        if (selected) {
            return (_focused) ? FOCUSED_STATE_FOCUSED_SELECTED : FOCUSED_STATE_UNFOCUSED_SELECTED;
        } else {
            return (_focused) ? FOCUSED_STATE_FOCUSED : FOCUSED_STATE_UNFOCUSED;
        }
    }

private:
    void getParentWindowAttributes(CreateWindowParams &params,
                                   Window child_window);
    void getChildWindowAttributes(CreateWindowParams &params,
                                  Window child_window);
    void createParentWindow(CreateWindowParams &params);
    void createTitle(CreateWindowParams &params);
    void createBorder(CreateWindowParams &params);

    void unloadDecor(void);

    ActionEvent *handleButtonPressButton(XButtonEvent *ev, PDecor::Button *button);
    ActionEvent *handleButtonPressBorder(XButtonEvent *ev);
    ActionEvent *handleButtonReleaseButton(XButtonEvent *ev, PDecor::Button *button);
    ActionEvent *handleButtonReleaseBorder(XButtonEvent *ev);

    EdgeType doMoveEdgeFind(int x, int y);
    void doMoveEdgeAction(XMotionEvent *ev, EdgeType edge);

    void placeButtons(void);
    void placeBorder(void);
    void shapeBorder(void);
    void restackBorder(void); // shaded, borderless, no border visible

    bool updateDecorName(void);

    void getBorderSize(BorderPosition pos, uint &width, uint &height);

    uint calcTitleWidth(void);
    void calcTabsWidth(void);
    void calcTabsGetAvailAndTabWidth(uint &width_avail, uint &tab_width, int &off);
    void calcTabsWidthSymetric(void);
    void calcTabsWidthAsymetric(void);
    void calcTabsWidthAsymetricGrow(uint width_avail, uint tab_width);
    void calcTabsWidthAsymetricShrink(uint width_avail, uint tab_width);

protected:
    Theme *_theme; //!< Pointer to Theme used by PDecor.
    std::string _decor_name; /**< Name of decoration used. */
    std::string _decor_name_override; /**< Overridden decoration name. */

    PWinObj *_child; //!< Pointer to active child in PDecor.
    vector<PWinObj*> _children; //!< List of children in PDecor.

    PDecor::Button *_button; /**< Active title button in PDecor. */
    Window _button_press_win; /**< Active border window, for button release handling. */

    // used for treshold calculation
    int _pointer_x; //!< Last click x root position.
    int _pointer_y; //!< Last click y root position.
    int _click_x; //!< Last click x window position.
    int _click_y; //!< Last click y window position.

    // how the decor should behave
    bool _decor_cfg_keep_empty; //!< Boolean to configure allowing empty PDecors.
    bool _decor_cfg_child_move_overloaded; //!< Boolean to set wheter ::move is overloaded.

    // button{press,release} handling cfg
    bool _decor_cfg_bpr_replay_pointer; //!< Boolean to configure wheter to call XReplayPointer on clicks.
    MouseActionListName _decor_cfg_bpr_al_child; //!< What list to search for child actions.
    MouseActionListName _decor_cfg_bpr_al_title; //!< What list to search for title actions.

    static const std::string DEFAULT_DECOR_NAME; //!< Default decor name in normal state.
    static const std::string DEFAULT_DECOR_NAME_BORDERLESS; //!< Default decor name in borderless state.
    static const std::string DEFAULT_DECOR_NAME_TITLEBARLESS; //!< Default decor name in titlebarless state.
    static const std::string DEFAULT_DECOR_NAME_ATTENTION; //!< Default decor name for demands attention state.

    // state switches, commonly not used by all decors
    bool _maximized_vert;
    bool _maximized_horz;
    bool _fullscreen;
    uint _skip;

    Window _border_win[BORDER_NO_POS]; /** Array of border windows. */
    std::map<BorderPosition, Pixmap> _border_pos_map;

private:
    Theme::PDecorData *_data;

    // decor state
    bool _border, _titlebar, _shaded;
    bool _need_shape;
    bool _need_client_shape;
    bool _dirty_resized; //!< Flag set when decor has been resized.
    uint _real_height;

    PWinObj _title_wo;
    vector<PDecor::Button*> _buttons;

    Pixmap _title_bg;

    // variable decor data
    uint _title_active;
    vector<PDecor::TitleItem*> _titles;
    uint _titles_left, _titles_right; // area where to put titles

    static vector<PDecor*> _pdecors; /**< List of all PDecors */
};

#endif // _PDECOR_HH_
