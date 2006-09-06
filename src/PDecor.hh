//
// PDecor.hh for pekwm
// Copyright (C) 2004-2006 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#include "../config.h"

#ifndef _PDECOR_HH_
#define _PDECOR_HH_

#include "Config.hh"
#include "Theme.hh" // for Theme::FrameData inlines
#include "PTexture.hh" // for border/image inlines
#include "Util.hh"

#include <list>

class ActionEvent;
class PFont;
class PWinObj;

//! @brief PWinObj container class with fancy decor.
class PDecor : public PWinObj
{
public:
    //! @brief Decor title button class.
class Button : public PWinObj {
    public:
        Button(Display *dpy, PWinObj *parent, Theme::PDecorButtonData *data);
        ~Button(void);

        ActionEvent *findAction(XButtonEvent *ev);
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

        inline const std::string &getVisible(void) const { return _visible; }
        inline const std::string &getReal(void) const { return _real; }
        inline const std::string &getCustom(void) const { return _custom; }
        inline const std::string &getUser(void) const { return _user; }
 
        inline uint getCount(void) const { return _count; }
        inline uint getId(void) const { return _id; }
        inline bool isUserSet(void) const { return (_user.size() > 0); }
        inline bool isCustom(void) const { return (_custom.size() > 0); }
        inline uint getWidth(void) const { return _width; }

        inline void infoAdd(enum Info info) { _info |= info; }
        inline void infoRemove(enum Info info) { _info &= ~info; }
        inline bool infoIs(enum Info info) { return (_info&info); }

        void setReal(const std::string &real) {
            _real = real;
            if (!isUserSet() && !isCustom()) {
                updateVisible();
            }
        }
        void setCustom(const std::string &custom) {
            _custom = custom;
            if (_custom.size() > 0 && !isUserSet()) {
                updateVisible();
            }
        }
        void setUser(const std::string &user) {
            _user = user;
            updateVisible();
        }
        void setCount(uint count) { _count = count; }
        void setId(uint id) { _id = id; }
        inline void setWidth(uint width) { _width = width; }

        void updateVisible(void);

    private:
        std::string _visible; //!< Visible version of title

        std::string _real; //!< Title from client
        std::string _custom; //!< Custom (title rule) set version of title
        std::string _user; //!< User set version of title

        uint _count; //!< Number of title
        uint _id; //!< ID of title
        uint _info; //!< Info bitmask for extra title info

        uint _width;
    };

    PDecor(Display *dpy, Theme *theme, const std::string decor_name = DEFAULT_DECOR_NAME);
    virtual ~PDecor(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void mapWindowRaised(void);
    virtual void unmapWindow(void);

    virtual void move(int x, int y, bool do_virtual = true);
    virtual void moveVirtual(int x, int y);
    virtual void resize(uint width, uint height);
    virtual void raise(void);
    virtual void lower(void);

    virtual void setFocused(bool focused);
    virtual void setWorkspace(uint workspace);

    virtual bool giveInputFocus(void);

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
    virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
    virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

    virtual bool operator == (const Window &window) {
        if ((_window == window) || (_title_wo == window) ||
                (findButton(window) != NULL) ||
                (getBorderPosition(window) != BORDER_NO_POS) ||
                ((_child != NULL) ? (*_child == window) : false)) {
            return true;
        }
        return false;
    }
    virtual bool operator != (const Window &window) {
        return !(*this == window);
    }
    // END - PWinObj interface.

    // START - PDecor interface.
    virtual void addChild(PWinObj *child);
    virtual void removeChild(PWinObj *child, bool do_delete = true);
    virtual void activateChild(PWinObj *child);

    virtual void updatedChildOrder(void) { }
    virtual void updatedActiveChild(void) { }

    virtual void getDecorInfo(char *buf, uint size);

    virtual void setShaded(StateAction sa);
    virtual void setSkip(uint skip);
    // END - PDecor interface.

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
    inline uint size(void) const { return _child_list.size(); }
    //! @brief Returns iterator to the first child in PDecor.
    inline std::list<PWinObj*>::iterator begin(void) {
        return _child_list.begin();
    }
    //! @brief Returns iterator to the last+1 child in PDecor.
    inline std::list<PWinObj*>::iterator end(void) { return _child_list.end(); }

    //! @brief Returns pointer to active PWinObj.
    inline PWinObj *getActiveChild(void) { return _child; }
    PWinObj *getChildFromPos(int x);

    void activateChildNum(uint num);
    void activateChildRel(int off); // +/- relative from active
    void moveChildRel(int off); // +/- relative from active

    // title

    //! @brief Adds TitleItem to title list.
    inline void titleAdd(PDecor::TitleItem *ct) { _title_list.push_back(ct); }
    //! @brief Removes all TitleItems from title list.
    inline void titleClear(void) { _title_list.clear(); }
    //! @brief Sets active TitleItem.
    void titleSetActive(uint num) {
        _title_active = (num > _title_list.size()) ? 0 : num;
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

    //! @brief Returns title height, 0 if titlebar disabled.
    inline uint getTitleHeight(void) const {
        return (_titlebar ? _data->getTitleHeight() : 0);
    }

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
    inline PFont *getFont(FocusedState state) { return _data->getFont(state); }

    uint getNearestHead(void);

    void checkSnap(void);
    void checkWOSnap(void);
    void checkEdgeSnap(void);

#ifdef HAVE_SHAPE
    bool setShape(void);
#endif // HAVE_SHAPE
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
    void unloadDecor(void);

    EdgeType doMoveEdgeFind(int x, int y);
    void doMoveEdgeAction(XMotionEvent *ev, EdgeType edge);

    void placeButtons(void);
    void placeBorder(void);
    void shapeBorder(void);
    void restackBorder(void); // shaded, borderless, no border visible

    bool updateDecorName(void);

    PWinObj *getChildRel(int off); // +/- relative from active
    void getBorderSize(BorderPosition pos, uint &width, uint &height);

    uint calcTitleWidth(void);
    void calcTabsWidth(void);
    void calcTabsWidthSymetric(void);
    void calcTabsWidthAsymetric(void);

protected:
    Theme *_theme; //!< Pointer to Theme used by PDecor.

    PWinObj *_child; //!< Pointer to active child in PDecor.
    std::list<PWinObj*> _child_list; //!< List of children in PDecor.

    PDecor::Button *_button; //!< Active title button in PDecor.

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

    // state switches, commonly not used by all decors
    bool _maximized_vert;
    bool _maximized_horz;
    bool _fullscreen;
    uint _skip;

private:
    Theme::PDecorData *_data;
    std::string _decor_name, _decor_name_override;

    // decor state
    bool _border, _titlebar, _shaded;
    bool _need_shape;
    bool _need_client_shape;
    uint _real_height;

    PWinObj _title_wo;
    std::list<PDecor::Button*> _button_list;

    Window _border_win[BORDER_NO_POS];
    std::list<Pixmap> _border_pix_list;

    Pixmap _title_bg;

    // variable decor data
    uint _title_active;
    std::list<PDecor::TitleItem*> _title_list;
    uint _titles_left, _titles_right; // area where to put titles
};

#endif // _PDECOR_HH_
