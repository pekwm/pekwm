//
// Frame.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _FRAME_HH_
#define _FRAME_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "Action.hh"
#include "PDecor.hh"

class PScreen;
class PWinObj;
class Strut;
class Theme;
class ClassHint;
class AutoProperty;

class Client;

#include <string>
#include <list>
#include <vector>

class Frame : public PDecor
{
public:
    Frame(Client *client, AutoProperty *ap);
    virtual ~Frame(void);

    // START - PWinObj interface.
    virtual void iconify(void);
    virtual void stick(void);

    virtual void setWorkspace(uint workspace);

    virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
    virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

    virtual ActionEvent *handleMapRequest(XMapRequestEvent *ev);
    virtual ActionEvent *handleUnmapEvent(XUnmapEvent *ev);
    // END - PWinObj interface.

#ifdef HAVE_SHAPE
    virtual void handleShapeEvent(XAnyEvent *ev);
#endif // HAVE_SHAPE

    // START - PDecor interface.
    virtual void addChild(PWinObj *child);
    virtual void removeChild(PWinObj *child, bool do_delete = true);
    virtual void activateChild(PWinObj *child);

    virtual void updatedChildOrder(void);
    virtual void updatedActiveChild(void);

    virtual void getDecorInfo(wchar_t *buf, uint size);

    virtual void setShaded(StateAction sa);
    virtual void setSkip(uint skip);
    // END - PDecor interface.

    static Frame *findFrameFromWindow(Window win);
    static Frame *findFrameFromID(uint id);

    // START - Iterators
    static uint frame_size(void) { return _frame_list.size(); }
    static std::list<Frame*>::iterator frame_begin(void) {
        return _frame_list.begin();
    }
    static std::list<Frame*>::iterator frame_end(void) {
        return _frame_list.end();
    }
    static std::list<Frame*>::reverse_iterator frame_rbegin(void) {
        return _frame_list.rbegin();
    }
    static std::list<Frame*>::reverse_iterator frame_rend(void) {
        return _frame_list.rend();
    }
    // END - Iterator

    inline uint getId(void) const { return _id; }
    void setId(uint id);

    void detachClient(Client *client);

    inline const ClassHint* getClassHint(void) const { return _class_hint; }

    void growDirection(uint direction);
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
    void setStateTitle(StateAction sa, Client *client, const std::wstring &title);
    void setStateMarked(StateAction sa, Client *client);

    void close(void);

    void readAutoprops(uint type = APPLY_ON_RELOAD);

    void doResize(XMotionEvent *ev); // redirects to doResize(bool...
    void doResize(BorderPosition pos); // redirect to doResize(bool...
    void doResize(bool left, bool x, bool top, bool y);
    void doGroupingDrag(XMotionEvent *ev, Client *client, bool behind);

    bool fixGeometry(void);

    // client message handling
    void handleConfigureRequest(XConfigureRequestEvent *ev, Client *client);
    void handleClientMessage(XClientMessageEvent *ev, Client *client);
    void handlePropertyChange(XPropertyEvent *ev, Client *client);

    static Frame *getTagFrame(void) { return _tag_frame; }
    static bool getTagBehind(void) { return _tag_behind; }

    static void resetFrameIDs(void);

protected:
    // BEGIN - PDecor interface
    virtual int resizeHorzStep(int diff) const;
    virtual int resizeVertStep(int diff) const;
    // END - PDecor interface

private:
    void handleConfigureRequestGeometry(XConfigureRequestEvent *ev, Client *client);
    void recalcResizeDrag(int nx, int ny, bool left, bool top);
    void getMaxBounds(int &max_x,int &max_r, int &max_y, int &max_b);
    void calcSizeInCells(uint &width, uint &height);
    void calcGravityPosition(int gravity, int x, int y, int &g_x, int &g_y);
    void downSize(bool keep_x, bool keep_y);

    void handleTitleChange(Client *client);

    void getState(Client *cl);
    void applyState(Client *cl);

    void setupAPGeometry(Client *client, AutoProperty *ap);
    void applyAPGeometry(Geometry &gm, const Geometry &ap_gm, int mask);

    void setActiveTitle(void);

    static uint findFrameID(void);
    static void returnFrameID(uint id);

    static std::string getClientDecorName(Client *client);

private:
    PScreen *_scr;

    uint _id; // unique id of the frame

    Client *_client; // to skip all the casts from PWinObj
    ClassHint *_class_hint;

    // frame information used when maximizing / going fullscreen
    Geometry _old_gm; // FIXME: move to PDecor?
    uint _non_fullscreen_decor_state; // FIXME: move to PDecor?
    uint _non_fullscreen_layer;

    // ID list, list of free Frame ids.
    static std::list<Frame*> _frame_list; //!< List of all Frames.
    static std::vector<uint> _frameid_list; //!< List of free Frame IDs.

    // Tagging, static as only one Frame can be tagged
    static Frame *_tag_frame; //!< Pointer to tagged frame.
    static bool _tag_behind; //!< Tagging actions will set behind.

    // EWMH
    static const int NET_WM_STATE_REMOVE = 0; // remove/unset property
    static const int NET_WM_STATE_ADD = 1; // add/set property
    static const int NET_WM_STATE_TOGGLE = 2; // toggle property
};

#endif // _FRAME_HH_
