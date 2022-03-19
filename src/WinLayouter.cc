//
// WinLayouter.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
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
#include "X11Util.hh"
#include "X11.hh"

static PWinObj*
isEmptySpace(int x, int y, const PWinObj* wo, std::vector<PWinObj*> &wvec)
{
	if (! wo) {
		return 0;
	}

	if (wvec.empty()) {
		// say that it's placed, now check if we are wrong!
		Workspaces::iterator it(Workspaces::begin());
		Workspaces::iterator end(Workspaces::end());
		for (; it != end; ++it) {
			// Skip ourselves, non-mapped and desktop objects. Iconified means
			// skip placement.
			if (wo == (*it) || ! (*it)->isMapped() || (*it)->isIconified()
			    || ((*it)->getLayer() == LAYER_DESKTOP)) {
				continue;
			}

			// Also skip windows tagged as Maximized as they cause us to
			// automatically fail.
			if ((*it)->getType() == PWinObj::WO_FRAME) {
				Client *client = static_cast<Frame*>((*it))->getActiveClient();
				if (client &&
				    (client->isFullscreen()
				     || (client->isMaximizedVert() && client->isMaximizedHorz()))) {
					continue;
				}
			}

			wvec.push_back(*it);
		}
	}

	for (unsigned i=0; i < wvec.size(); ++i) {
		// Check if we are "intruding" on some other window's place
		if ((wvec[i]->getX() < signed(x + wo->getWidth())) &&
		    (signed(wvec[i]->getX() + wvec[i]->getWidth()) > x) &&
		    (wvec[i]->getY() < signed(y + wo->getHeight())) &&
		    (signed(wvec[i]->getY() + wvec[i]->getHeight()) > y)) {
			return wvec[i];
		}
	}

	return 0; // we passed the test, no frames in the way
}

//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
class LayouterSmart : public WinLayouter {
public:
	LayouterSmart() : WinLayouter() {}
	virtual ~LayouterSmart() {}

private:
	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		PWinObj *wo_e;
		bool placed = false;
		std::vector<PWinObj*> wvec;

		int step_x = (pekwm::config()->getPlacementLtR()) ? 1 : -1;
		int step_y = (pekwm::config()->getPlacementTtB()) ? 1 : -1;
		int offset_x = (pekwm::config()->getPlacementLtR())
			? pekwm::config()->getPlacementOffsetX()
			: -pekwm::config()->getPlacementOffsetX();
		int offset_y = (pekwm::config()->getPlacementTtB())
			? pekwm::config()->getPlacementOffsetY()
			: -pekwm::config()->getPlacementOffsetY();
		int start_x, start_y, test_x = 0, test_y = 0;

		// Wrap these up, to get proper checking of space.
		uint wo_width = wo->getWidth() + pekwm::config()->getPlacementOffsetX();
		uint wo_height = wo->getHeight() + pekwm::config()->getPlacementOffsetY();

		start_x = pekwm::config()->getPlacementLtR() ?
			head_gm.x : head_gm.x + head_gm.width - wo_width;
		start_y = pekwm::config()->getPlacementTtB() ?
			head_gm.y : head_gm.y + head_gm.height - wo_height;

		if (pekwm::config()->getPlacementRow()) { // row placement
			test_y = start_y;
			while (! placed && (pekwm::config()->getPlacementTtB()
					    ? test_y + wo_height <= head_gm.y + head_gm.height
					    : test_y >= head_gm.y)) {
				test_x = start_x;
				while (! placed && (pekwm::config()->getPlacementLtR()
						    ? test_x + wo_width <= head_gm.x + head_gm.width
						    : test_x >= head_gm.x)) {
					// see if we can place the window here
					if ((wo_e = isEmptySpace(test_x, test_y, wo, wvec))) {
						placed = false;
						test_x = pekwm::config()->getPlacementLtR() ?
							wo_e->getX() + wo_e->getWidth() : wo_e->getX() - wo_width;
					} else {
						placed = true;
						wo->move(test_x + offset_x, test_y + offset_y);
					}
				}
				test_y += step_y;
			}
		} else { // column placement
			test_x = start_x;
			while (! placed && (pekwm::config()->getPlacementLtR()
					    ? test_x + wo_width <= head_gm.x + head_gm.width
					    : test_x >= head_gm.x)) {
				test_y = start_y;
				while (! placed && (pekwm::config()->getPlacementTtB()
						    ? test_y + wo_height <= head_gm.y + head_gm.height
						    : test_y >= head_gm.y)) {
					// see if we can place the window here
					if ((wo_e = isEmptySpace(test_x, test_y, wo, wvec))) {
						placed = false;
						test_y = pekwm::config()->getPlacementTtB() ?
							wo_e->getY() + wo_e->getHeight() : wo_e->getY() - wo_height;
					} else {
						placed = true;
						wo->move(test_x + offset_x, test_y + offset_y);
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
	LayouterCentered(void) : WinLayouter() {}
	virtual ~LayouterCentered(void) { };

	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)

	{
		Geometry gm = head_gm.center(wo->getGeometry());
		wo->move(gm.x, gm.y);
		return true;
	}
};

//! @brief Places the wo in a corner of the screen not under the pointer
class LayouterMouseNotUnder : public WinLayouter {
public:
	LayouterMouseNotUnder() : WinLayouter() {}
	virtual ~LayouterMouseNotUnder() {}

	virtual bool layout(PWinObj *wo, Window parent,
			    const Geometry &head_gm, int ptr_x, int ptr_y)
	{
		// compensate for head offset
		ptr_x -= head_gm.x;
		ptr_y -= head_gm.y;

		// divide the screen into four rectangles using the pointer as divider
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

//! @brief Places the client centered under the mouse
class LayouterMouseCentred : public WinLayouter {
public:
	LayouterMouseCentred() : WinLayouter() {}
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

//! @brief Places the client like the menu gets placed
class LayouterMouseTopLeft : public WinLayouter {
public:
	LayouterMouseTopLeft() : WinLayouter() {}

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

WinLayouter*
WinLayouterFactory(std::string name)
{
	std::string name_upper(name);
	Util::to_upper(name_upper);

	if (! name_upper.compare("SMART")) {
		return new LayouterSmart;
	}
	if (! name_upper.compare("CENTERED")) {
		return new LayouterCentered;
	}
	if (! name_upper.compare("MOUSENOTUNDER")) {
		return new LayouterMouseNotUnder;
	}
	if (! name_upper.compare("MOUSECENTERED")) {
		return new LayouterMouseCentred;
	}
	if (! name_upper.compare("MOUSETOPLEFT")) {
		return new LayouterMouseTopLeft;
	}
	USER_WARN("Unknown placement model: " << name);
	return nullptr;
}
