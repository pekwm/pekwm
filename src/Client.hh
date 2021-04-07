//
// Client.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// client.hh for aewm++
// Copyright (C) 2002 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "Observable.hh"
#include "PWinObj.hh"
#include "PTexturePlain.hh"
#include "PDecor.hh"

class PScreen;
class Strut;
class ClassHint;
class AutoProperty;
class Frame;

#include <string>

extern "C" {
#include <X11/Xutil.h>
}

class LayerObservation : public Observation {
public:
    LayerObservation(enum Layer _layer) : layer(_layer) { };
    virtual ~LayerObservation(void) { };
public:
    const enum Layer layer; /**< Layer client changed to. */
};

class ClientInitConfig {
public:
    bool focus;
    bool focus_parent;
    bool map;
    bool parent_is_new;
};

class Client : public PWinObj,
               public Observer
{
    // FIXME: This relationship should end as soon as possible, but I need to
    // figure out a good way of sharing. :)
    friend class Frame;

public: // Public Member Functions
    Client(Window new_client, ClientInitConfig &initConfig,
           bool is_new = false);
    virtual ~Client(void);

    // START - PWinObj interface.
    virtual void mapWindow(void) override;
    virtual void unmapWindow(void) override;
    virtual void iconify(void) override;
    virtual void stick(void) override;

    virtual void move(int x, int y) override;
    virtual void resize(uint width, uint height) override;
    virtual void moveResize(int x, int y, uint width, uint height) override;

    virtual void setWorkspace(uint workspace) override;

    virtual void giveInputFocus(void) override;
    virtual void reparent(PWinObj *parent, int x, int y) override;

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev)  override {
        if (_parent) { return _parent->handleButtonPress(ev); }
        return 0;
    }
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev) override {
        if (_parent) { return _parent->handleButtonRelease(ev); }
        return 0;
    }

    virtual ActionEvent *handleMapRequest(XMapRequestEvent *ev) override;
    virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev) override;
    // END - PWinObj interface.

#ifdef HAVE_SHAPE
    void handleShapeEvent(XShapeEvent *);
#endif // HAVE_SHAPE

    // START - Observer interface.
    virtual void notify(Observable *observable,
                        Observation *observation) override;
    // END - Observer interface.

    static Client *findClient(Window win);
    static Client *findClientFromWindow(Window win);
    static Client *findClientFromHint(const ClassHint *class_hint);
    static Client *findClientFromID(uint id);
    static void findFamilyFromWindow(std::vector<Client*> &client_list,
                                     Window win);

    static void mapOrUnmapTransients(Window win, bool hide);

    // START - Iterators
    static uint client_size(void) { return _clients.size(); }
    static std::vector<Client*>::const_iterator client_begin(void) {
        return _clients.begin();
    }
    static std::vector<Client*>::const_iterator client_end(void) {
        return _clients.end();
    }
    static std::vector<Client*>::const_reverse_iterator client_rbegin(void) {
        return _clients.rbegin();
    }
    static std::vector<Client*>::const_reverse_iterator client_rend(void) {
        return _clients.rend();
    }

    bool hasTransients() const { return ! _transients.empty(); }
    std::vector<Client*>::const_iterator getTransientsBegin(void) const {
        return _transients.begin();
    }
    std::vector<Client*>::const_iterator getTransientsEnd(void) const {
        return _transients.end();
    }
    // END - Iterators

    bool validate(void);

    inline uint getClientID(void) { return _id; }
    /**< Return title item for client name. */
    inline PDecor::TitleItem *getTitle(void) { return &_title; }

    inline const ClassHint* getClassHint(void) const { return _class_hint; }

    bool isTransient(void) const { return _transient_for_window != None; }
    Client *getTransientForClient(void) const { return _transient_for; }
    Window getTransientForClientWindow(void) const {
        return _transient_for_window;
    }
    void findAndRaiseIfTransient(void);

    XSizeHints getActiveSizeHints(void) const {
        auto hints = *_size;
        if (isCfgDeny(CFG_DENY_RESIZE_INC)) {
            hints.flags &= ~PResizeInc;
        }
        return hints;
    }

    bool isViewable(void);
    bool cameWithPosition(void) {
        return _size->flags & (PPosition|USPosition);
    }

    bool hasTitlebar(void) const { return (_state.decor&DECOR_TITLEBAR); }
    bool hasBorder(void) const { return (_state.decor&DECOR_BORDER); }
    bool hasStrut(void) const { return (_strut); }
    Strut *getStrut(void) const { return _strut; }
    bool demandsAttention(void) const { return _demands_attention; }

    PTexture *getIcon(void) const { return _icon; }

    /** Return PID of client. */
    long getPid(void) const { return _pid; }
    /** Return true if client is remote. */
    bool isRemote(void) const { return _is_remote; }

    // State accessors
    bool isMaximizedVert(void) const { return _state.maximized_vert; }
    bool isMaximizedHorz(void) const { return _state.maximized_horz; }
    bool isShaded(void) const { return _state.shaded; }
    bool isFullscreen(void) const override { return _state.fullscreen; }
    bool isPlaced(void) const { return _state.placed; }
    uint getInitialFrameOrder(void) const { return _state.initial_frame_order; }
    uint getSkip(void) const { return _state.skip; }
    bool isSkip(Skip skip) const { return (_state.skip&skip); }
    uint getDecorState(void) const { return _state.decor; }
    bool isCfgDeny(uint deny) const { return (_state.cfg_deny&deny); }

    bool allowMove(void) const { return _actions.move; }
    bool allowResize(void) const { return _actions.resize; }
    bool allowIconify(void) const { return _actions.iconify; }
    bool allowShade(void) const { return _actions.shade; }
    bool allowStick(void) const { return _actions.stick; }
    bool allowMaximizeHorz(void) const { return _actions.maximize_horz; }
    bool allowMaximizeVert(void) const { return _actions.maximize_vert; }
    bool allowFullscreen(void) const { return _actions.fullscreen; }
    bool allowChangeWorkspace(void) const { return _actions.change_ws; }
    bool allowClose(void) const { return _actions.close; }

    bool isAlive(void) const { return _alive; }
    bool isMarked(void) const { return _marked; }

    void grabButtons(void);

    void setStateCfgDeny(StateAction sa, uint deny);
    inline void setStateMarked(StateAction sa) {
        if (ActionUtil::needToggle(sa, _marked)) {
            _marked = !_marked;
            if (_marked) {
                _title.infoAdd(PDecor::TitleItem::INFO_MARKED);
            } else {
                _title.infoRemove(PDecor::TitleItem::INFO_MARKED);
            }
            _title.updateVisible();
        }
    }

    // toggles
    void alwaysOnTop(bool top);
    void alwaysBelow(bool bottom);

    void setSkip(uint skip);

    inline void setStateSkip(StateAction sa, Skip skip) {
        if ((isSkip(skip) && (sa == STATE_SET))
            || (! isSkip(skip) && (sa == STATE_UNSET))) {
            return;
        }
        _state.skip ^= skip;
    }

    inline void setTitlebar(bool titlebar) {
        if (titlebar) {
            _state.decor |= DECOR_TITLEBAR;
        } else {
            _state.decor &= ~DECOR_TITLEBAR;
        }
    }

    inline void setBorder(bool border) {
        if (border) {
            _state.decor |= DECOR_BORDER;
        } else {
            _state.decor &= ~DECOR_BORDER;
        }
    }

    inline void setDemandsAttention(bool attention) {
        _demands_attention = attention;
    }

    std::string getAPDecorName(void);

    void close(void);
    void kill(void);

    void handleDestroyEvent(XDestroyWindowEvent *ev);
    void handleColormapChange(XColormapEvent *ev);

    inline bool setConfigureRequestLock(bool lock) {
        bool old_lock = _cfg_request_lock;
        _cfg_request_lock = lock;
        return old_lock;
    }

    void configureRequestSend(void);
    void sendTakeFocusMessage(void);

    bool getAspectSize(uint *r_w, uint *r_h, uint w, uint h);
    bool getIncSize(const XSizeHints& size,
                    uint *r_w, uint *r_h, uint w, uint h, bool incr=false);
    void updateEwmhStates(void);

    inline AtomName getWinType(void) const { return _window_type; }
    void updateWinType(bool set);

    ulong getWMHints(void);
    void getWMNormalHints(void);
    void getWMProtocols(void);
    void getTransientForHint(void);
    void updateParentLayerAndRaiseIfActive(void);
    void getStrutHint(void);
    void readName(void);
    void removeStrutHint(void);

    long getPekwmFrameOrder(void);
    void setPekwmFrameOrder(long num);
    bool getPekwmFrameActive(void);
    void setPekwmFrameActive(bool active);

    static void setClientEnvironment(Client *client);
    AutoProperty* readAutoprops(ApplyOn type = APPLY_ON_ALWAYS);

private:
    bool getAndUpdateWindowAttributes(void);

    bool findOrCreateFrame(AutoProperty *autoproperty);
    bool findTaggedFrame(void);
    bool findPreviousFrame(void);
    bool findAutoGroupFrame(AutoProperty *autoproperty);

    void setInitialState(void);
    void setClientInitConfig(ClientInitConfig &initConfig, bool is_new,
                             AutoProperty *autoproperty);

    bool titleApplyRule(std::wstring &wtitle);
    uint titleFindID(std::wstring &wtitle);

    void setWmState(ulong state);
    long getWmState(void);

    // these are used by frame
    inline void setMaximizedVert(bool m) { _state.maximized_vert = m; }
    inline void setMaximizedHorz(bool m) { _state.maximized_horz = m; }
    inline void setShade(bool s) { _state.shaded = s; }
    inline void setFullscreen(bool f) { _state.fullscreen = f; }

    void readHints(void);
    void readClassRoleHints(void);
    void readEwmhHints(void);
    void readMwmHints(void);
    void readPekwmHints(void);
    void readIcon(void);
    void applyAutoprops(AutoProperty *ap);
    void applyActionAccessMask(uint mask, bool value);
    void readClientPid(void);
    void readClientRemote(void);

    void setTransientFor(Client *client);
    void addTransient(Client *client);
    void removeTransient(Client *client);

    static uint findClientID(void);
    static void returnClientID(uint id);

private: // Private Member Variables
    uint _id; //<! Unique ID of the Client.

    XSizeHints *_size;
    Colormap _cmap;

    /** Client for which this client is transient for */
    Client *_transient_for;
    Window _transient_for_window;
    std::vector<Client*> _transients; /**< Vector of transient clients. */

    Strut *_strut;

    PDecor::TitleItem _title; /**< Name of the client. */
    PTextureImage *_icon;

    /** _NET_WM_PID of the client, only valid if is_remote is false. */
    Cardinal _pid;
    bool _is_remote;

    ClassHint *_class_hint;
    AtomName _window_type; /**< _NET_WM_WINDOW_TYPE */

    bool _alive, _marked;
    bool _send_focus_message, _send_close_message, _wm_hints_input;
    bool _cfg_request_lock;
    bool _extended_net_name;
    /** If true, the client requires attention from the user. */
    bool _demands_attention;

    class State {
    public:
        State(void)
            : maximized_vert(false),
              maximized_horz(false),
              shaded(false),
              fullscreen(false),
              placed(false),
              initial_frame_order(0),
              skip(0),
              decor(DECOR_TITLEBAR|DECOR_BORDER),
              cfg_deny(CFG_DENY_NO)
        {
        }
        ~State(void) { }

        bool maximized_vert, maximized_horz;
        bool shaded;
        bool fullscreen;

        // pekwm states
        bool placed;
        uint initial_frame_order; /**< Initial frame position */
        uint skip, decor, cfg_deny;
    } _state;

    class Actions {
    public:
        Actions(void) : move(true), resize(true), iconify(true),
                shade(true), stick(true), maximize_horz(true),
                maximize_vert(true), fullscreen(true),
                change_ws(true), close(true) { }
        ~Actions(void) { }

        bool move:1;
        bool resize:1;
        bool iconify:1; // iconify
        bool shade:1;
        bool stick:1;
        bool maximize_horz:1;
        bool maximize_vert:1;
        bool fullscreen:1;
        bool change_ws:1; // workspace
        bool close:1;
    } _actions;

    static const long _clientEventMask;

    static std::vector<Client*> _clients; //!< Vector of all Clients.
    static std::vector<uint> _clientids; //!< Vector of free Client IDs.
};
