//
// Workspaces.hh for pekwm
// Copyright © 2002-2009 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WORKSPACES_HH_
#define _WORKSPACES_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "x11.hh"

#include <string>
#include <list>
#include <vector>

class EwmhAtoms;
class PWinObj;
class Frame;
class FrameWidget;

class Workspaces {
public:
    class Workspace {
    public:
        Workspace(const std::wstring &name, uint number);
        ~Workspace(void);

        inline std::wstring &getName(void) { return _name; }
        void setName(const std::wstring &name) { _name = name; }
        inline uint getNumber(void) { return _number; }
        inline PWinObj* getLastFocused(void) { return _last_focused; }

        inline void setLastFocused(PWinObj* wo) { _last_focused = wo; }

    private:
        std::wstring _name;
        uint _number;

        PWinObj *_last_focused;
    };

    Workspaces(uint number, uint per_row);
    ~Workspaces(void);

    static inline Workspaces *instance(void) { return _instance; }

    inline uint size(void) const { return _workspace_list.size(); }
    inline std::list<PWinObj*>::iterator begin(void) { return _wo_list.begin(); }
    inline std::list<PWinObj*>::iterator end(void) { return _wo_list.end(); }
    inline std::list<PWinObj*>::reverse_iterator rbegin(void) { return _wo_list.rbegin(); }
    inline std::list<PWinObj*>::reverse_iterator rend(void) { return _wo_list.rend(); }

    std::vector<Workspace*>::iterator ws_begin(void) { return _workspace_list.begin(); }
    std::vector<Workspace*>::iterator ws_end(void) { return _workspace_list.end(); }

    inline uint getActive(void) const { return _active; }
    inline uint getPrevious(void) const { return _previous; }
    uint getRow(int active = -1) {
        if (active < 0) {
            active = _active;
        }
        return _per_row ? (active / _per_row) : 0; 
    }
    uint getRowMin(void) { return _per_row ? (getRow() * _per_row) : 0; }
    uint getRowMax(void) { return _per_row ? (getRowMin() + _per_row - 1) : size() - 1; }
    uint getRows(void) { return _per_row ? (size() / _per_row + (size() % _per_row ? 1 : 0)) : 1; }
    uint getPerRow(void) { return _per_row ? _per_row : size(); }

    void setSize(uint number);
    void setPerRow(uint per_row) { _per_row = per_row; }
    void setNames(void);

    void setWorkspace(uint num, bool focus);
    bool gotoWorkspace(uint direction, bool warp);

    inline const std::list<PWinObj*> &getWOList(void) const {
        return _wo_list;
    }

    Workspace *getActiveWorkspace(void) {
      return _workspace_list[_active];
    }
    Workspace *getWorkspace(uint workspace) {
      if (workspace >= _workspace_list.size())
        return 0;
      return _workspace_list[workspace];
    };

    void insert(PWinObj* wo, bool raise = true);
    void remove(PWinObj* wo);

    void hideAll(uint workspace);
    void unhideAll(uint workspace, bool focus);

    PWinObj* getLastFocused(uint workspace);
    void setLastFocused(uint workspace, PWinObj* wo);

    void raise(PWinObj* wo);
    void lower(PWinObj* wo);
    void stackAbove(PWinObj* wo, Window win, bool restack = true);
    void stackBelow(PWinObj *wo, Window win, bool restack = true);

    PWinObj* getTopWO(uint type_mask);
    void updateClientList(void);
    void updateClientStackingList(void);
    void placeWo(PWinObj* wo, Window parent);
    void placeWoInsideScreen(PWinObj *wo);

    PWinObj *findDirectional(PWinObj *wo, DirectionType dir, uint skip = 0);

private:
    Window *buildClientList(unsigned int &num_windows);
    bool warpToWorkspace(uint num, int dir);

    void stackWinUnderWin(Window over, Window under);

    std::wstring getWorkspaceName(uint num);

    // placement
    bool placeSmart(PWinObj* wo);
    bool placeMouseNotUnder(PWinObj *wo);
    bool placeMouseCentered(PWinObj *wo);
    bool placeMouseTopLeft(PWinObj *wo);
    bool placeCenteredOnParent(PWinObj *wo, Window parent);
    void placeInsideScreen(Geometry &gm, Strut *strut=0);

    // placement helpers
    PWinObj* isEmptySpace(int x, int y, const PWinObj *wo);
    inline bool isBetween(const int &x1, const int &x2,
                          const int &t1, const int &t2) {
        if (x1 > t1) {
            if (x1 < t2)
                return true;
        } else if (x2 > t1) {
            return true;
        }
        return false;
    }

private:
    static Workspaces *_instance;

    uint _active; /**< Current active workspace. */
    uint _previous; /**< Previous workspace. */
    uint _per_row; /**< Workspaces per row in layout. */

    std::list<PWinObj*> _wo_list;
    std::vector<Workspace*> _workspace_list;
};

#endif // _WORKSPACES_HH_
