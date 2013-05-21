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

#include <string>

class Strut;
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

    typedef std::vector<PWinObj*>::iterator iterator;
    typedef std::vector<PWinObj*>::const_iterator const_iterator;
    typedef std::vector<PWinObj*>::reverse_iterator reverse_iterator;
    typedef std::vector<PWinObj*>::const_reverse_iterator const_reverse_iterator;

    static void free(void);

    static inline uint size(void) { return _workspace_list.size(); }
    static inline iterator begin(void) { return _wobjs.begin(); }
    static inline iterator end(void) { return _wobjs.end(); }
    static inline reverse_iterator rbegin(void) { return _wobjs.rbegin(); }
    static inline reverse_iterator rend(void) { return _wobjs.rend(); }

    static vector<Workspace*>::iterator ws_begin(void) { return _workspace_list.begin(); }
    static vector<Workspace*>::iterator ws_end(void) { return _workspace_list.end(); }

    static inline uint getActive(void) { return _active; }
    static inline uint getPrevious(void) { return _previous; }
    static uint getRow(int active = -1) {
        if (active < 0) {
            active = _active;
        }
        return _per_row ? (active / _per_row) : 0; 
    }
    static uint getRowMin(void) { return _per_row ? (getRow() * _per_row) : 0; }
    static uint getRowMax(void) { return _per_row ? (getRowMin() + _per_row - 1) : size() - 1; }
    static uint getRows(void) { return _per_row ? (size() / _per_row + (size() % _per_row ? 1 : 0)) : 1; }
    static uint getPerRow(void) { return _per_row ? _per_row : size(); }

    static void setSize(uint number);
    static void setPerRow(uint per_row) { _per_row = per_row; }
    static void setNames(void);

    static void setWorkspace(uint num, bool focus);
    static bool gotoWorkspace(uint direction, bool warp);

    static Workspace *getActiveWorkspace(void) {
        return _workspace_list[_active];
    }

    static void insert(PWinObj* wo, bool raise = true);
    static void remove(PWinObj* wo);

    static void hideAll(uint workspace);
    static void unhideAll(uint workspace, bool focus);

    static PWinObj* getLastFocused(uint workspace);
    static void setLastFocused(uint workspace, PWinObj* wo);

    static void raise(PWinObj* wo);
    static void lower(PWinObj* wo);

    static PWinObj* getTopWO(uint type_mask);
    static void updateClientList(void);
    static void updateClientStackingList(void);
    static void placeWo(PWinObj *wo, Window parent);
    static void placeWoInsideScreen(PWinObj *wo);

    static PWinObj *findDirectional(PWinObj *wo, DirectionType dir, uint skip = 0);

private:
    static Window *buildClientList(unsigned int &num_windows);
    static bool warpToWorkspace(uint num, int dir);

    static void stackWinUnderWin(Window over, Window under);

    static std::wstring getWorkspaceName(uint num);

    // placement
    static bool placeWo(PWinObj *wo, uint head_num, Window parent);
    static bool placeSmart(PWinObj *wo, const Geometry &head);
    static bool placeMouseNotUnder(PWinObj *wo, const Geometry &head);
    static bool placeMouseCentered(PWinObj *wo);
    static bool placeMouseTopLeft(PWinObj *wo);
    static bool placeCenteredOnParent(PWinObj *wo, Window parent);

    // placement helpers
    static PWinObj* isEmptySpace(int x, int y, const PWinObj *wo);

    static uint _active; /**< Current active workspace. */
    static uint _previous; /**< Previous workspace. */
    static uint _per_row; /**< Workspaces per row in layout. */

    static vector<PWinObj*> _wobjs;
    static vector<Workspace*> _workspace_list;
};

#endif // _WORKSPACES_HH_
