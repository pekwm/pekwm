//
// WinLayouter.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"

#include "Client.hh"
#include "Debug.hh"
#include "Frame.hh"
#include "Util.hh"
#include "ManagerWindows.hh"
#include "WinLayouter.hh"
#include "Workspaces.hh"
#include "X11.hh"

static Util::StringTo<WinLayouterType> win_layouter_type_map[] =
	{{"SMART", WIN_LAYOUTER_SMART},
	 {"CENTERED", WIN_LAYOUTER_CENTERED},
	 {"CENTEREDONPARENT", WIN_LAYOUTER_CENTEREDONPARENT},
	 {"MOUSENOTUNDER", WIN_LAYOUTER_MOUSENOTUNDER},
	 {"MOUSECENTERED", WIN_LAYOUTER_MOUSECENTERED},
	 {"MOUSETOPLEFT", WIN_LAYOUTER_MOUSECTOPLEFT},
	 {nullptr, WIN_LAYOUTER_NO}};

static void
populateWvec(const PWinObj *wo, std::vector<PWinObj*> &wvec)
{
	Workspaces::iterator it(Workspaces::begin());
	Workspaces::iterator end(Workspaces::end());
	for (; it != end; ++it) {
		// Include standard window objects excluding the provded wo.
		// Windows that are not mapped, desktop windows and iconified
		// windows are excludes.
		enum PWinObj::Type it_type = (*it)->getType();
		if (wo == (*it)
		    || ! (*it)->isMapped()
		    || ! (it_type == PWinObj::WO_FRAME
			  || it_type == PWinObj::WO_MENU)
		    || ((*it)->getLayer() == LAYER_DESKTOP)) {
			continue;
		}

		// Skip windows tagged as Maximized as they cause no space
		// to be found.
		if ((*it)->isType(PWinObj::WO_FRAME)) {
			Frame *frame = static_cast<Frame*>(*it);
			Client *client = frame->getActiveClient();
			if (client &&
			    (client->isFullscreen()
			     || (client->isMaximizedVert()
				 && client->isMaximizedHorz()))) {
				continue;
			}
		}

		wvec.push_back(*it);
	}
}

static PWinObj*
isEmptySpace(int x, int y, const PWinObj* wo, std::vector<PWinObj*> &wvec)
{
	if (wo == nullptr) {
		return nullptr;
	}

	if (wvec.empty()) {
		populateWvec(wo, wvec);
	}

	for (unsigned i=0; i < wvec.size(); ++i) {
		// Check if window is on some other windows space
		if ((wvec[i]->getX() < signed(x + wo->getWidth())) &&
		    (signed(wvec[i]->getX() + wvec[i]->getWidth()) > x) &&
		    (wvec[i]->getY() < signed(y + wo->getHeight())) &&
		    (signed(wvec[i]->getY() + wvec[i]->getHeight()) > y)) {
			return wvec[i];
		}
	}

	return nullptr;
}

/**
 * Window layouter that tries to find empty space to place the client in
 * @return true if client got placed, else false
 */
class LayouterSmart : public WinLayouter {
public:
	LayouterSmart() : WinLayouter(WIN_LAYOUTER_SMART) {}
	virtual ~LayouterSmart() {}

private:
	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		PWinObj *wo_e;
		bool placed = false;
		std::vector<PWinObj*> wvec;
		Config* cfg = pekwm::config();

		int step_x = (cfg->getPlacementLtR()) ? 1 : -1;
		int step_y = (cfg->getPlacementTtB()) ? 1 : -1;
		int offset_x = (cfg->getPlacementLtR())
			? cfg->getPlacementOffsetX()
			: -cfg->getPlacementOffsetX();
		int offset_y = (cfg->getPlacementTtB())
			? cfg->getPlacementOffsetY()
			: -cfg->getPlacementOffsetY();
		int start_x, start_y, test_x, test_y;

		// Wrap these up, to get proper checking of space.
		uint wo_width = wo->getWidth() + cfg->getPlacementOffsetX();
		uint wo_height = wo->getHeight() + cfg->getPlacementOffsetY();

		start_x = cfg->getPlacementLtR() ?
			head_gm.x : head_gm.x + head_gm.width - wo_width;
		start_y = cfg->getPlacementTtB() ?
			head_gm.y : head_gm.y + head_gm.height - wo_height;

		if (cfg->getPlacementRow()) { // row placement
			test_y = start_y;
			while (! placed
			       && (cfg->getPlacementTtB()
				   ? test_y + wo_height
				     <= head_gm.y + head_gm.height
				   : test_y >= head_gm.y)) {
				test_x = start_x;
				while (! placed
				       && (cfg->getPlacementLtR()
					   ? test_x + wo_width
					     <= head_gm.x + head_gm.width
					   : test_x >= head_gm.x)) {
					// see if we can place the window here
					wo_e = isEmptySpace(test_x, test_y,
							    wo, wvec);
					if (wo_e) {
						placed = false;
						test_x = cfg->getPlacementLtR()
							? wo_e->getX()
							  + wo_e->getWidth()
							: wo_e->getX()
							  - wo_width;
					} else {
						placed = true;
						wo->move(test_x + offset_x,
							 test_y + offset_y);
					}
				}
				test_y += step_y;
			}
		} else { // column placement
			test_x = start_x;
			while (! placed
			       && (cfg->getPlacementLtR()
				   ? test_x + wo_width
				     <= head_gm.x + head_gm.width
				   : test_x >= head_gm.x)) {
				test_y = start_y;
				while (! placed
				       && (cfg->getPlacementTtB()
					   ? test_y + wo_height
					     <= head_gm.y + head_gm.height
					   : test_y >= head_gm.y)) {
					// see if we can place the window here
					wo_e = isEmptySpace(test_x, test_y,
							    wo, wvec);
					if (wo_e) {
						placed = false;
						test_y = cfg->getPlacementTtB()
							? wo_e->getY()
							  + wo_e->getHeight()
							: wo_e->getY()
							  - wo_height;
					} else {
						placed = true;
						wo->move(test_x + offset_x,
							 test_y + offset_y);
					}
				}
				test_x += step_x;
			}
		}
		return placed;
	}
};

/**
 * Place windows centered on the current head.
 */
class LayouterCentered : public WinLayouter {
public:
	LayouterCentered(void) : WinLayouter(WIN_LAYOUTER_CENTERED) {}
	virtual ~LayouterCentered(void) { };

	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)

	{
		Geometry gm = head_gm.center(wo->getGeometry());
		wo->move(gm.x, gm.y);
		return true;
	}
};

/**
 * Place window centered on its parent (excluding root as parent)
 */
class LayouterCenteredOnParent : public WinLayouter {
public:
	LayouterCenteredOnParent(void)
		: WinLayouter(WIN_LAYOUTER_CENTEREDONPARENT)
	{
	}
	virtual ~LayouterCenteredOnParent(void) { };

	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)

	{
		if (parent == None || parent == X11::getRoot()) {
			return false;
		}

		PWinObj *parent_wo = PWinObj::findPWinObj(parent);
		if (parent_wo != nullptr) {
			const Geometry &parent_gm = parent_wo->getGeometry();
			Geometry gm = parent_gm.center(wo->getGeometry());
			wo->move(gm.x, gm.y);
			return true;
		}
		return false;
	}
};

/**
 * Place the window in one of the screen corners where it does not end up under
 * the pointer.
 */
class LayouterMouseNotUnder : public WinLayouter {
public:
	LayouterMouseNotUnder() : WinLayouter(WIN_LAYOUTER_MOUSENOTUNDER) {}
	virtual ~LayouterMouseNotUnder() {}

	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		// compensate for head offset
		ptr_x -= head_gm.x;
		ptr_y -= head_gm.y;

		// divide the screen into four rectangles using the pointer as
		// divider
		if (wo->getWidth() < unsigned(ptr_x)
		    && wo->getHeight() < head_gm.height) {
			wo->move(head_gm.x, head_gm.y);
			return true;
		}

		if (wo->getWidth() < head_gm.width
		    && wo->getHeight() < unsigned(ptr_y)) {
			wo->move(head_gm.x + head_gm.width - wo->getWidth(),
				 head_gm.y);
			return true;
		}

		if (wo->getWidth() < head_gm.width - ptr_x
		    && wo->getHeight() < head_gm.height) {
			wo->move(head_gm.x + head_gm.width - wo->getWidth(),
				 head_gm.y + head_gm.height - wo->getHeight());
			return true;
		}

		if (wo->getWidth() < head_gm.width
		    && wo->getHeight() < head_gm.height - ptr_y) {
			wo->move(head_gm.x,
				 head_gm.y + head_gm.height - wo->getHeight());
			return true;
		}
		return false;
	}
};

/**
 * Places the window centered under the mouse
 */
class LayouterMouseCentred : public WinLayouter {
public:
	LayouterMouseCentred() : WinLayouter(WIN_LAYOUTER_MOUSECENTERED) {}
	~LayouterMouseCentred() {}

private:
	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		Geometry gm(ptr_x - (wo->getWidth() / 2),
			    ptr_y - (wo->getHeight() / 2),
			    wo->getWidth(),
			    wo->getHeight());
		pekwm::rootWo()->placeInsideScreen(gm);
		wo->move(gm.x, gm.y);
		return true;
	}
};

/**
 * Places the window like the menu gets placed, top left corner under the
 * pointer.
 */
class LayouterMouseTopLeft : public WinLayouter {
public:
	LayouterMouseTopLeft() : WinLayouter(WIN_LAYOUTER_MOUSECTOPLEFT) {}

private:
	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		Geometry gm(ptr_x, ptr_y, wo->getWidth(), wo->getHeight());
		pekwm::rootWo()->placeInsideScreen(gm);
		wo->move(gm.x, gm.y);
		return true;
	}
};

enum WinLayouterType
win_layouter_type_from_string(const std::string &name)
{
	enum WinLayouterType type =
		Util::StringToGet(win_layouter_type_map, name);
	if (type == WIN_LAYOUTER_NO) {
		USER_WARN("Unknown placement model: " << name);
	}
	return type;
}

/**
 * Construct a WinLayouter of the provided name.
 */
WinLayouter*
mkWinLayouter(const std::string &name)
{
	return mkWinLayouter(win_layouter_type_from_string(name));
}

/**
 * Construct a WinLayouter of the provided type, caller is responsible for
 * freeing the returned layouter.
 */
WinLayouter*
mkWinLayouter(enum WinLayouterType type)
{
	switch (type) {
	case WIN_LAYOUTER_SMART:
		return new LayouterSmart();
	case WIN_LAYOUTER_CENTERED:
		return new LayouterCentered();
	case WIN_LAYOUTER_CENTEREDONPARENT:
		return new LayouterCenteredOnParent();
	case WIN_LAYOUTER_MOUSENOTUNDER:
		return new LayouterMouseNotUnder();
	case WIN_LAYOUTER_MOUSECENTERED:
		return new LayouterMouseCentred();
	case WIN_LAYOUTER_MOUSECTOPLEFT:
		return new LayouterMouseTopLeft();
	default:
		return nullptr;
	}
}
