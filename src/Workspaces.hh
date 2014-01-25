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

#include <string>

#include "pekwm.hh"
#include "WinLayouter.hh"

class PWinObj;
class Frame;

class Workspace {
public:
    Workspace() : _name(), _layouter(0), _last_focused(0) { }
    Workspace(const Workspace &w) : _name(w._name), _layouter(w._layouter),
          _last_focused(w._last_focused)
    {
        w._layouter = 0;
    }
    ~Workspace(void);
    Workspace &operator=(const Workspace &w);

    inline const std::wstring &getName(void) const { return _name; }
    inline void setName(const std::wstring &name) { _name = name; }

    inline WinLayouter *getLayouter(void) const {
        return _layouter?_layouter:_default_layouter;
    }
    void setLayouter(WinLayouter *);
    static void setDefaultLayouter(const std::string &);

    inline PWinObj* getLastFocused(void) const { return _last_focused; }
    inline void setLastFocused(PWinObj* wo) { _last_focused = wo; }

private:
    std::wstring _name;
    mutable WinLayouter *_layouter; // evil hack, will change with C++11
    PWinObj *_last_focused;

    static WinLayouter *_default_layouter;
};

class Workspaces {
public:
    typedef std::vector<PWinObj*>::iterator iterator;
    typedef std::vector<PWinObj*>::const_iterator const_iterator;
    typedef std::vector<PWinObj*>::reverse_iterator reverse_iterator;
    typedef std::vector<PWinObj*>::const_reverse_iterator const_reverse_iterator;

    static inline iterator begin(void) { return _wobjs.begin(); }
    static inline iterator end(void) { return _wobjs.end(); }
    static inline reverse_iterator rbegin(void) { return _wobjs.rbegin(); }
    static inline reverse_iterator rend(void) { return _wobjs.rend(); }

    static inline uint size(void) { return _workspaces.size(); }
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

    static Workspace &getActWorkspace(void) {
        return _workspaces[_active];
    }

    static void layout(Frame *f=0, Window parent=None) {
        _workspaces[_active].getLayouter()->layout(f, parent);
    }
    static void layoutOnce(const std::string &layouter);
    static void setLayouter(uint workspace, const std::string &layouter);
    static void setLayouterOption(const std::string &option, Frame *);
    static bool isTiling(void) { return isTiling(_active); }
    static bool isTiling(uint ws) {
        return ws < _workspaces.size()?_workspaces[ws].getLayouter()->isTiling():false;
    }

    static void layoutIfTiling(Frame *f=0) {
        if (isTiling(_active)) {
            layout(f);
        }
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
    static void placeWoInsideScreen(PWinObj *wo);

    static PWinObj *findDirectional(PWinObj *wo, DirectionType dir, uint skip = 0);

    static void fixStacking(PWinObj *);

private:
    static Window *buildClientList(unsigned int &num_windows);
    static bool warpToWorkspace(uint num, int dir);

    static std::wstring getWorkspaceName(uint num);

    static uint _active; /**< Current active workspace. */
    static uint _previous; /**< Previous workspace. */
    static uint _per_row; /**< Workspaces per row in layout. */

    static vector<PWinObj*> _wobjs;
    static vector<Workspace> _workspaces;
};

#endif // _WORKSPACES_HH_
