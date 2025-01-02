//
// Workspaces.hh for pekwm
// Copyright (C) 2002-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_WORKSPACES_HH_
#define _PEKWM_WORKSPACES_HH_

#include "config.h"

#include <string>

#include "pekwm.hh"
#include "WinLayouter.hh"
#include "WorkspaceIndicator.hh"

class PWinObj;
class Frame;

class Workspace {
public:
	Workspace(void);
	Workspace(const Workspace &w);
	~Workspace(void);
	Workspace &operator=(const Workspace &w);

	inline const std::string &getName(void) const { return _name; }
	inline void setName(const std::string &name) { _name = name; }

	PWinObj* getLastFocused(bool verify) const;
	void setLastFocused(PWinObj* wo);

private:
	std::string _name;
	PWinObj *_last_focused;
};

class Workspaces {
public:
	typedef std::vector<PWinObj*>::iterator iterator;
	typedef std::vector<PWinObj*>::const_iterator const_iterator;
	typedef std::vector<PWinObj*>::reverse_iterator reverse_iterator;
	typedef std::vector<PWinObj*>::const_reverse_iterator
	const_reverse_iterator;

	static void init(void);
	static void cleanup(void);

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
	static uint getRowMin(void) {
		return _per_row ? (getRow() * _per_row) : 0;
	}
	static uint getRowMax(void) {
		return _per_row ? (getRowMin() + _per_row - 1) : size() - 1;
	}
	static uint getRows(void) {
		if (! _per_row) {
			return 1;
		}
		return size() / _per_row + ((size() % _per_row) ? 1 : 0);
	}
	static uint getPerRow(void) { return _per_row ? _per_row : size(); }

	static void setSize(uint number);
	static void setPerRow(uint per_row) { _per_row = per_row; }
	static void setNames(void);

	static void setLayoutModels(const std::vector<std::string> &models);

	static void setWorkspace(uint num, bool focus);
	static bool gotoWorkspace(uint direction, bool focus, bool warp);

	static Workspace &getActWorkspace(void) {
		return _workspaces[_active];
	}

	static void layout(Frame *frame, Window parent=None);
	static void insert(PWinObj* wo, bool raise = true);
	static void remove(const PWinObj* wo);

	static void hideAll(uint workspace);
	static void unhideAll(uint workspace, bool focus);

	static PWinObj* getLastFocused(uint workspace);
	static void setLastFocused(uint workspace, PWinObj* wo);

	static bool raise(PWinObj* wo);
	static bool lower(PWinObj* wo);
	static void restack(PWinObj* wo, PWinObj* sibling, long detail);
	static bool handleFullscreenBeforeRaise(PWinObj* wo);
	static bool isOccluding(const PWinObj* wo, const PWinObj* sibling);

	static PWinObj* getTopFocusableWO(uint type_mask);
	static void updateClientList(void);
	static void updateClientStackingList(void);
	static void placeWoInsideScreen(PWinObj *wo);

	static void findWOAndFocus(PWinObj *search);
	static PWinObj *findUnderPointer(void);
	static PWinObj *findDirectional(PWinObj *wo,
					DirectionType dir, uint skip = 0);
	static Frame* getNextFrame(Frame* frame, bool mapped, uint mask = 0);
	static Frame* getPrevFrame(Frame* frame, bool mapped, uint mask = 0);

	static void fixStacking(PWinObj *);

	static void showWorkspaceIndicator(void);
	static void hideWorkspaceIndicator(void);

	// list iterators
	static std::vector<Frame*>::iterator mru_begin(void) {
		return _mru.begin();
	}
	static std::vector<Frame*>::iterator mru_end(void) {
		return _mru.end();
	}

	// adds
	static void addToMRUFront(Frame *frame) {
		if (frame) {
			removeFromMRU(frame);
			_mru.insert(_mru.begin(), frame);
		}
	}

	static void addToMRUBack(Frame *frame) {
		if (frame) {
			removeFromMRU(frame);
			_mru.push_back(frame);
		}
	}

	static void removeFromMRU(Frame *frame) {
		_mru.erase(std::remove(_mru.begin(), _mru.end(), frame),
			   _mru.end());
	}

protected:
	static PWinObj *findWOAndFocusFind(bool stacking);

	static bool restackTopIf(PWinObj* wo);
	static bool restackBottomIf(PWinObj* wo);
	static bool swapInStack(PWinObj* wo_under, PWinObj* wo_over);
	static bool stackAbove(PWinObj* wo_under, PWinObj* wo_over);

	static iterator find(const PWinObj *wo);

	/** All PWinObjs, the highest stacked PWinObj is kept at the back. */
	static std::vector<PWinObj*> _wobjs;
	/** The most recently used frame is kept at the front. */
	static std::vector<Frame*> _mru;

private:
	static PWinObj *findWOAndFocusStacking();
	static PWinObj *findWOAndFocusMRU();
	static void findWOAndFocusRaise(PWinObj *wo);

	static uint overlapPercent(PWinObj *wo);

	static bool restack(PWinObj *wo, long detail);
	static bool restackSibling(PWinObj *wo, PWinObj *sibling, long detail);
	static void stackAt(iterator it);

	static void clearLayoutModels(void);
	static bool layoutOnHead(PWinObj *wo, Window parent,
				 const Geometry &gm, int ptr_x, int ptr_y);

	static Window *buildClientList(unsigned int &num_windows);
	static bool warpToWorkspace(uint num, int dir);

	static bool lowerFullscreenWindows(Layer new_layer);
	static std::string getWorkspaceName(uint num);

	static uint _active; /**< Current active workspace. */
	static uint _previous; /**< Previous workspace. */
	static uint _per_row; /**< Workspaces per row in layout. */

	/** List of layouters tried in sequence when placing windows. */
	static std::vector<WinLayouter*> _layout_models;

	/** Window popping up when switching workspace */
	static WorkspaceIndicator *_workspace_indicator;

	static std::vector<Workspace> _workspaces;
};

#endif // _PEKWM_WORKSPACES_HH_
