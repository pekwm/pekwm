//
// Client.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// client.hh for aewm++
// Copyright (C) 2002 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CLIENT_HH_
#define _CLIENT_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "PWinObj.hh"
#include "Observer.hh"
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

class Client : public PWinObj, public Observer
{
    // FIXME: This relationship should end as soon as possible, but I need to
    // figure out a good way of sharing. :)
    friend class Frame;

public: // Public Member Functions
    struct MwmHints {
        ulong flags;
        ulong functions;
        ulong decorations;
    };
    enum {
        MWM_HINTS_FUNCTIONS = (1L << 0),
        MWM_HINTS_DECORATIONS = (1L << 1),
        MWM_HINTS_NUM = 3
    };
    enum {
        MWM_FUNC_ALL = (1L << 0),
        MWM_FUNC_RESIZE = (1L << 1),
        MWM_FUNC_MOVE = (1L << 2),
        MWM_FUNC_ICONIFY = (1L << 3),
        MWM_FUNC_MAXIMIZE = (1L << 4),
        MWM_FUNC_CLOSE = (1L << 5)
    };
    enum {
        MWM_DECOR_ALL = (1L << 0),
        MWM_DECOR_BORDER = (1L << 1),
        MWM_DECOR_HANDLE = (1L << 2),
        MWM_DECOR_TITLE = (1L << 3),
        MWM_DECOR_MENU = (1L << 4),
        MWM_DECOR_ICONIFY = (1L << 5),
        MWM_DECOR_MAXIMIZE = (1L << 6)
    };

    Client(Window new_client, ClientInitConfig &initConfig, bool is_new = false);
    virtual ~Client(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    virtual void iconify(void);
    virtual void stick(void);

    virtual void move(int x, int y);
    virtual void resize(uint width, uint height);
    virtual void moveResize(int x, int y, uint width, uint height);

    virtual void setWorkspace(uint workspace);

    virtual void giveInputFocus(void);
    virtual void reparent(PWinObj *parent, int x, int y);

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev) {
        if (_parent) { return _parent->handleButtonPress(ev); }
        return 0;
    }
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev) {
        if (_parent) { return _parent->handleButtonRelease(ev); }
        return 0;
    }

    virtual ActionEvent *handleMapRequest(XMapRequestEvent *ev);
    virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev);
    // END - PWinObj interface.

#ifdef HAVE_SHAPE
    void handleShapeEvent(XShapeEvent *);
#endif // HAVE_SHAPE

    // START - Observer interface.
    virtual void notify(Observable *observable, Observation *observation);
    // END - Observer interface.

    static Client *findClient(Window win);
    static Client *findClientFromWindow(Window win);
    static Client *findClientFromHint(const ClassHint *class_hint);
    static Client *findClientFromID(uint id);
    static void findFamilyFromWindow(vector<Client*> &client_list, Window win);

    static void mapOrUnmapTransients(Window win, bool hide);

    // START - Iterators
    static uint client_size(void) { return _clients.size(); }
    static vector<Client*>::const_iterator client_begin(void) {
        return _clients.begin();
    }
    static vector<Client*>::const_iterator client_end(void) {
        return _clients.end();
    }
    static vector<Client*>::const_reverse_iterator client_rbegin(void) {
        return _clients.rbegin();
    }
    static vector<Client*>::const_reverse_iterator client_rend(void) {
        return _clients.rend();
    }

    unsigned int transient_size(void) { return _transients.size(); }
    vector<Client*>::const_iterator transient_begin(void) {
        return _transients.begin(); }
    vector<Client*>::const_iterator transient_end(void) {
        return _transients.end(); }
    // END - Iterators

    bool validate(void);

    inline uint getClientID(void) { return _id; }
    /**< Return title item for client name. */
    inline PDecor::TitleItem *getTitle(void) { return &_title; }

    inline const ClassHint* getClassHint(void) const { return _class_hint; }

    bool isTransient(void) const { return _transient_for_window != None; }
    Client *getTransientForClient(void) const { return _transient_for; }
    Window getTransientForClientWindow(void) const { return _transient_for_window; }
    void findAndRaiseIfTransient(void);

    inline XSizeHints* getXSizeHints(void) const { return _size; }

    bool isViewable(void);
    bool setPUPosition(void);

    inline bool hasTitlebar(void) const { return (_state.decor&DECOR_TITLEBAR); }
    inline bool hasBorder(void) const { return (_state.decor&DECOR_BORDER); }
    inline bool hasStrut(void) const { return (_strut); }
    Strut *getStrut(void) const { return _strut; }
    inline bool demandsAttention(void) const { return _demands_attention; }

    PTexture *getIcon(void) const { return _icon; }

    /** Return PID of client. */
    long getPid(void) const { return _pid; }
    /** Return true if client is remote. */
    bool isRemote(void) const { return _is_remote; }

    // State accessors
    inline bool isMaximizedVert(void) const { return _state.maximized_vert; }
    inline bool isMaximizedHorz(void) const { return _state.maximized_horz; }
    inline bool isShaded(void) const { return _state.shaded; }
    inline bool isFullscreen(void) const { return _state.fullscreen; }
    inline bool isPlaced(void) const { return _state.placed; }
    inline uint getInitialFrameOrder(void) const { return _state.initial_frame_order; }
    inline uint getSkip(void) const { return _state.skip; }
    inline bool isSkip(Skip skip) const { return (_state.skip&skip); }
    inline uint getDecorState(void) const { return _state.decor; }
    inline bool isCfgDeny(uint deny) { return (_state.cfg_deny&deny); }

    inline bool allowMove(void) const { return _actions.move; }
    inline bool allowResize(void) const { return _actions.resize; }
    inline bool allowIconify(void) const { return _actions.iconify; }
    inline bool allowShade(void) const { return _actions.shade; }
    inline bool allowStick(void) const { return _actions.stick; }
    inline bool allowMaximizeHorz(void) const { return _actions.maximize_horz; }
    inline bool allowMaximizeVert(void) const { return _actions.maximize_vert; }
    inline bool allowFullscreen(void) const { return _actions.fullscreen; }
    inline bool allowChangeWorkspace(void) const { return _actions.change_ws; }
    inline bool allowClose(void) const { return _actions.close; }

    inline bool isAlive(void) const { return _alive; }
    inline bool isMarked(void) const { return _marked; }

    // We have this public so that we can reload button actions.
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
        if ((isSkip(skip) && (sa == STATE_SET)) || (! isSkip(skip) && (sa == STATE_UNSET))) {
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

    void close(void);
    void kill(void);

    // Event handlers below - Used by WindowManager
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
    bool getIncSize(uint *r_w, uint *r_h, uint w, uint h, bool incr=false);

    bool getEwmhStates(NetWMStates &win_states);
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
    void setClientInitConfig(ClientInitConfig &initConfig, bool is_new, AutoProperty *autoproperty);

    bool titleApplyRule(std::wstring &wtitle);
    uint titleFindID(std::wstring &wtitle);

    void setWmState(ulong state);
    long getWmState(void);

    MwmHints* getMwmHints(Window w);

    // these are used by frame
    inline void setMaximizedVert(bool m) { _state.maximized_vert = m; }
    inline void setMaximizedHorz(bool m) { _state.maximized_horz = m; }
    inline void setShade(bool s) { _state.shaded = s; }
    inline void setFullscreen(bool f) { _state.fullscreen = f; }
    inline void setFocusable(bool f) { _focusable = f; }

    // Grabs button with Caps,Num and so on
    void grabButton(int button, int mod, int mask, Window win);

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

    static uint findClientID(void);
    static void returnClientID(uint id);

private: // Private Member Variables
    uint _id; //<! Unique ID of the Client.

    XSizeHints *_size;
    Colormap _cmap;

    Client *_transient_for; /**< Client for which this client is transient for */
    Window _transient_for_window;
    vector<Client*> _transients; /**< Vector of transient clients. */

    Strut *_strut;

    PDecor::TitleItem _title; /**< Name of the client. */
    PTextureImage *_icon;
    
    long _pid; /**< _NET_WM_PID of the client, only valid if is_remote is false. */
    bool _is_remote; /**< Boolean flag  */

    ClassHint *_class_hint;
    AtomName _window_type; /**< _NET_WM_WINDOW_TYPE */

    bool _alive, _marked;
    bool _send_focus_message, _send_close_message, _wm_hints_input;
    bool _cfg_request_lock;
    bool _extended_net_name;
    bool _demands_attention; /**< If true, the client requires attention from the user. */

    class State {
    public:
        State(void)
            : maximized_vert(false), maximized_horz(false), shaded(false), fullscreen(false),
              placed(false), initial_frame_order(0),
              skip(0), decor(DECOR_TITLEBAR|DECOR_BORDER),
              cfg_deny(CFG_DENY_NO) { }
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

    static vector<Client*> _clients; //!< Vector of all Clients.
    static vector<uint> _clientids; //!< Vector of free Client IDs.
};

#endif // _CLIENT_HH_
