//
// Workspaces.hh for pekwm
// Copyright (C) 2002-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _WORKSPACES_HH_
#define _WORKSPACES_HH_

#include "pekwm.hh"

#include <string>
#include <list>
#include <vector>

class PScreen;
class EwmhAtoms;
class PWinObj;
class Frame;
class FrameWidget;
class Viewport;

class Workspaces {
public:
    class Workspace {
    public:
        Workspace(const std::string &name, uint number,
                  const std::list<PWinObj*> &wo_list);
        ~Workspace(void);

        inline std::string& getName(void) { return _name; }
        inline uint getNumber(void) { return _number; }
        inline Viewport *getViewport(void) { return _viewport; }
        inline PWinObj* getLastFocused(void) { return _last_focused; }

        inline void setLastFocused(PWinObj* wo) { _last_focused = wo; }

    private:
        std::string _name;
        uint _number;
        Viewport *_viewport;

        const std::list<PWinObj*> &_wo_list;
        PWinObj *_last_focused;
    };

    Workspaces(uint number);
    ~Workspaces(void);

    static inline Workspaces *instance(void) { return _instance; }

    inline uint size(void) const { return _workspace_list.size(); }
    inline std::list<PWinObj*>::iterator begin(void) { return _wo_list.begin(); }
    inline std::list<PWinObj*>::iterator end(void) { return _wo_list.end(); }
    inline std::list<PWinObj*>::reverse_iterator rbegin(void) { return _wo_list.rbegin(); }
    inline std::list<PWinObj*>::reverse_iterator rend(void) { return _wo_list.rend(); }

    inline uint getActive(void) const { return _active; }
    inline uint getPrevious(void) const { return _previous; }

    void setSize(uint number);

    void setWorkspace(uint num, bool focus);
    bool gotoWorkspace(uint direction, bool warp);

    inline const std::list<PWinObj*> &getWOList(void) const {
        return _wo_list;
    }

    inline Viewport *getActiveViewport(void) {
        return _workspace_list[_active]->getViewport();
    }
    inline Viewport *getViewport(uint workspace) {
        if (workspace >= _workspace_list.size())
            return NULL;
        return _workspace_list[workspace]->getViewport();
    }

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
    void updateClientStackingList(bool client, bool stacking);
    void placeWo(PWinObj* wo, Window parent);

    PWinObj *findDirectional(PWinObj *wo, DirectionType dir, uint skip = 0);

private:
    bool warpToWorkspace(uint num, int dir);

    void stackWinUnderWin(Window over, Window under);

    // placement
    bool placeSmart(PWinObj* wo);
    bool placeMouseNotUnder(PWinObj *wo);
    bool placeMouseCentered(PWinObj *wo);
    bool placeMouseTopLeft(PWinObj *wo);
    bool placeCenteredOnParent(PWinObj *wo, Window parent);
    void placeInsideScreen(Geometry &gm);

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

    std::list<PWinObj*> _wo_list;
    std::vector<Workspace*> _workspace_list;
};

#endif // _WORKSPACES_HH_
