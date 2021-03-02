//
// Frame.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "Action.hh"
#include "PDecor.hh"
#include "Client.hh"

class PWinObj;
class Strut;
class ClassHint;
class AutoProperty;

#include <string>

class Frame : public PDecor
{
public:
    Frame(Client *client, AutoProperty *ap);
    virtual ~Frame(void);

    // START - PWinObj interface.
    virtual void iconify(void) override;
    virtual void stick(void) override;

    virtual void setWorkspace(unsigned int workspace) override;
    virtual void setLayer(Layer layer) override;

    virtual ActionEvent *handleMotionEvent(XMotionEvent *ev) override;
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev) override;
    virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev) override;

    virtual ActionEvent *handleMapRequest(XMapRequestEvent *ev) override;
    virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev) override;
    // END - PWinObj interface.

#ifdef HAVE_SHAPE
    void handleShapeEvent(XShapeEvent *ev);
#endif // HAVE_SHAPE

    // START - PDecor interface.
    virtual bool allowMove(void) const override;

    virtual void addChild(PWinObj *child,
                          std::vector<PWinObj*>::iterator *it = 0) override;
    virtual void removeChild(PWinObj *child, bool do_delete = true) override;
    virtual void activateChild(PWinObj *child) override;

    virtual void updatedChildOrder(void) override;
    virtual void updatedActiveChild(void) override;

    virtual void getDecorInfo(wchar_t *buf, uint size,
                              const Geometry& gm) override;

    virtual void giveInputFocus(void) override;
    virtual void setShaded(StateAction sa) override;
    virtual void setSkip(uint skip) override;
    // END - PDecor interface.

    Client *getActiveClient(void);

    void addChildOrdered(Client *child);

    static Frame *findFrameFromWindow(Window win);
    static Frame *findFrameFromID(uint id);

    // START - Iterators
    static uint frame_size(void) { return _frames.size(); }
    static std::vector<Frame*>::const_iterator frame_begin(void) {
        return _frames.begin();
    }
    static std::vector<Frame*>::const_iterator frame_end(void) {
        return _frames.end();
    }
    static std::vector<Frame*>::const_reverse_iterator frame_rbegin(void) {
        return _frames.rbegin();
    }
    static std::vector<Frame*>::const_reverse_iterator frame_rend(void) {
        return _frames.rend();
    }

    Client *getTransFor(void) const {
        return _client?_client->getTransientForClient():0;
    }
    bool hasTrans(void) const {
        return _client && _client->hasTransients();
    }
    // Call getTransBegin() only (!) if hasTrans() == true.
    std::vector<Client*>::const_iterator getTransBegin(void) const {
        return _client->getTransientsBegin();
    }
    // Call getTransEnd() only (!) if hasTrans() == true.
    std::vector<Client*>::const_iterator getTransEnd(void) const {
        return _client->getTransientsEnd();
    }
    // END - Iterator

    inline uint getId(void) const { return _id; }
    void setId(uint id);

    void detachClient(Client *client);

    inline const ClassHint* getClassHint(void) const { return _class_hint; }

    void setGeometry(const std::string geometry, int head=-1,
                     bool honour_strut=false);

    void growDirection(uint direction);
    void moveToHead(int head_nr);
    void moveToEdge(OrientationType ori);

    void updateInactiveChildInfo(void);

    // state actions
    void setStateMaximized(StateAction sa, bool horz, bool vert, bool fill);
    void setStateFullscreen(StateAction sa);
    void setStateSticky(StateAction sa);
    void setStateAlwaysOnTop(StateAction sa);
    void setStateAlwaysBelow(StateAction sa);
    void setStateDecorBorder(StateAction sa);
    void setStateDecorTitlebar(StateAction sa);
    void setStateIconified(StateAction sa);
    void setStateTagged(StateAction sa, bool behind);
    void setStateSkip(StateAction sa, uint skip);
    void setStateTitle(StateAction sa, Client *client,
                       const std::wstring &title);
    void setStateMarked(StateAction sa, Client *client);
    void setStateOpaque(StateAction sa);

    void close(void);

    void readAutoprops(ApplyOn type = APPLY_ON_RELOAD);

    void doResize(XMotionEvent *ev); // redirects to doResize(bool...
    void doResize(BorderPosition pos); // redirect to doResize(bool...
    void doResize(bool left, bool x, bool top, bool y);
    void doGroupingDrag(XMotionEvent *ev, Client *client, bool behind);

    bool fixGeometry(void);

    // client message handling
    void handleConfigureRequest(XConfigureRequestEvent *ev, Client *client);
    ActionEvent *handleClientMessage(XClientMessageEvent *ev, Client *client);
    void handlePropertyChange(XPropertyEvent *ev, Client *client);

    static Frame *getTagFrame(void) { return _tag_frame; }
    static bool getTagBehind(void) { return _tag_behind; }

    static void resetFrameIDs(void);

protected:
    // used for testing
    Frame(void);

    // BEGIN - PDecor interface
    virtual int resizeHorzStep(int diff) const override;
    virtual int resizeVertStep(int diff) const override;

    virtual std::string getDecorName(void) override;
    // END - PDecor interface

    static void applyGeometry(Geometry &gm, const Geometry &ap_gm, int mask);
    static void applyGeometry(Geometry &gm, const Geometry &ap_gm, int mask,
                              const Geometry &screen_gm);

private:
    void handleClientStateMessage(XClientMessageEvent *ev, Client *client);
    static StateAction getStateActionFromMessage(XClientMessageEvent *ev);
    void handleStateAtom(StateAction sa, Atom atom, Client *client);
    void handleCurrentClientStateAtom(StateAction sa, Atom atom,
                                      Client *client);
    bool isRequestGeometryFullscreen(XConfigureRequestEvent *ev);

    void recalcResizeDrag(int nx, int ny, bool left, bool top);
    void getMaxBounds(int &max_x,int &max_r, int &max_y, int &max_b);
    void calcSizeInCells(uint &width, uint &height, const Geometry& gm);
    void setGravityPosition(int gravity, int &x, int &y,
                            int diff_w, int diff_h);
    void downSize(Geometry &gm, bool keep_x, bool keep_y);

    void handleTitleChange(Client *client, bool read_name);

    void getState(Client *cl);
    void applyState(Client *cl);

    void setupAPGeometry(Client *client, AutoProperty *ap);

    void workspacesInsert();
    void workspacesRemove();

    static uint findFrameID(void);
    static void returnFrameID(uint id);

private:
    uint _id; // unique id of the frame

    Client *_client; // to skip all the casts from PWinObj
    ClassHint *_class_hint;

    // frame information used when maximizing / going fullscreen
    Geometry _old_gm; // FIXME: move to PDecor?
    uint _non_fullscreen_decor_state; // FIXME: move to PDecor?
    Layer _non_fullscreen_layer;

    static std::vector<Frame*> _frames; //!< Vector of all Frames.
    static std::vector<uint> _frameid_list; //!< Vector of free Frame IDs.

    static ActionEvent _ae_move;
    static ActionEvent _ae_move_resize;

    // Tagging, static as only one Frame can be tagged
    static Frame *_tag_frame; //!< Pointer to tagged frame.
    static bool _tag_behind; //!< Tagging actions will set behind.

    // EWMH
    static const int NET_WM_STATE_REMOVE = 0; // remove/unset property
    static const int NET_WM_STATE_ADD = 1; // add/set property
    static const int NET_WM_STATE_TOGGLE = 2; // toggle property
};
