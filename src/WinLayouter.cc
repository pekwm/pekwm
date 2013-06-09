//
// WinLayouter.cc for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "WinLayouter.hh"
#include "LayouterTiling.hh"

#include "Client.hh"
#include "Frame.hh"
#include "Util.hh"
#include "WindowManager.hh"
#include "Workspaces.hh"
#include "x11.hh"

using std::string;

static PWinObj*
isEmptySpace(int x, int y, const PWinObj* wo, vector<PWinObj*> &wvec)
{
    if (! wo) {
        return 0;
    }

    if (wvec.empty()) {
        // say that it's placed, now check if we are wrong!
        vector<PWinObj*>::iterator it(Workspaces::begin());
        vector<PWinObj*>::iterator end(Workspaces::end());
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
    LayouterSmart() : WinLayouter(false) {}
    virtual ~LayouterSmart() {}

private:
    virtual bool layout_impl(Frame *wo)
    {
        if (! wo) {
            return true;
        }

        PWinObj *wo_e;
        bool placed = false;
        vector<PWinObj*> wvec;

        int step_x = (Config::instance()->getPlacementLtR()) ? 1 : -1;
        int step_y = (Config::instance()->getPlacementTtB()) ? 1 : -1;
        int offset_x = (Config::instance()->getPlacementLtR())
                       ? Config::instance()->getPlacementOffsetX()
                       : -Config::instance()->getPlacementOffsetX();
        int offset_y = (Config::instance()->getPlacementTtB())
                       ? Config::instance()->getPlacementOffsetY()
                       : -Config::instance()->getPlacementOffsetY();
        int start_x, start_y, test_x = 0, test_y = 0;

        // Wrap these up, to get proper checking of space.
        uint wo_width = wo->getWidth() + Config::instance()->getPlacementOffsetX();
        uint wo_height = wo->getHeight() + Config::instance()->getPlacementOffsetY();

        start_x = Config::instance()->getPlacementLtR() ?
                  _gm.x : _gm.x + _gm.width - wo_width;
        start_y = Config::instance()->getPlacementTtB() ?
                  _gm.y : _gm.y + _gm.height - wo_height;

        if (Config::instance()->getPlacementRow()) { // row placement
            test_y = start_y;
            while (! placed && (Config::instance()->getPlacementTtB()
                               ? test_y + wo_height <= _gm.y + _gm.height
                               : test_y >= _gm.y)) {
                test_x = start_x;
                while (! placed && (Config::instance()->getPlacementLtR()
                                    ? test_x + wo_width <= _gm.x + _gm.width
                                    : test_x >= _gm.x)) {
                    // see if we can place the window here
                    if ((wo_e = isEmptySpace(test_x, test_y, wo, wvec))) {
                        placed = false;
                        test_x = Config::instance()->getPlacementLtR() ?
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
            while (! placed && (Config::instance()->getPlacementLtR()
                                ? test_x + wo_width <= _gm.x + _gm.width
                                : test_x >= _gm.x)) {
                test_y = start_y;
                while (! placed && (Config::instance()->getPlacementTtB()
                                    ? test_y + wo_height <= _gm.y + _gm.height
                                    : test_y >= _gm.y)) {
                    // see if we can place the window here
                    if ((wo_e = isEmptySpace(test_x, test_y, wo, wvec))) {
                        placed = false;
                        test_y = Config::instance()->getPlacementTtB() ?
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

//! @brief Places the wo in a corner of the screen not under the pointer
class LayouterMouseNotUnder : public WinLayouter {
public:
    LayouterMouseNotUnder() : WinLayouter(false) {}
    virtual ~LayouterMouseNotUnder() {}

    virtual bool layout_impl(Frame *wo)
    {
        if (! wo) {
            return true;
        }

        // compensate for head offset
        _ptr_x -= _gm.x;
        _ptr_y -= _gm.y;

        // divide the screen into four rectangles using the pointer as divider
        if (wo->getWidth() < unsigned(_ptr_x) && wo->getHeight() < _gm.height) {
            wo->move(_gm.x, _gm.y);
            return true;
        }

        if (wo->getWidth() < _gm.width && wo->getHeight() < unsigned(_ptr_y)) {
            wo->move(_gm.x + _gm.width - wo->getWidth(), _gm.y);
            return true;
        }

        if (wo->getWidth() < _gm.width - _ptr_x && wo->getHeight() < _gm.height) {
            wo->move(_gm.x + _gm.width - wo->getWidth(), _gm.y + _gm.height - wo->getHeight());
            return true;
        }

        if (wo->getWidth() < _gm.width && wo->getHeight() < _gm.height - _ptr_y) {
            wo->move(_gm.x, _gm.y + _gm.height - wo->getHeight());
            return true;
        }
        return false;
    }
};

//! @brief Places the client centered under the mouse
class LayouterMouseCentred : public WinLayouter {
public:
    LayouterMouseCentred() : WinLayouter(false) {}
    ~LayouterMouseCentred() {}

private:
    virtual bool layout_impl(Frame *wo)
    {
        if (wo) {
            Geometry gm(_ptr_x - (wo->getWidth() / 2), _ptr_y - (wo->getHeight() / 2),
                        wo->getWidth(), wo->getHeight());

            // make sure it's within the screen's border
            X11::placeInsideScreen(gm);
            wo->move(gm.x, gm.y);
        }
        return true;
    }
};

//! @brief Places the client like the menu gets placed
class LayouterMouseTopLeft : public WinLayouter {
public:
    LayouterMouseTopLeft() : WinLayouter(false) {}

private:
    virtual bool layout_impl(Frame *wo)
    {
        if (wo) {
            Geometry gm(_ptr_x, _ptr_y, wo->getWidth(), wo->getHeight());
            X11::placeInsideScreen(gm); // make sure it's within the screen's border
            wo->move(gm.x, gm.y);
        }
        return true;
    }
};

void
WinLayouter::layout(Frame *frame, Window parent)
{
    if (frame) {
        frame->updateDecor();
    }

    if (_tiling) {
        layout_impl(frame);
        return;
    }

    if (frame && parent != None
              && Config::instance()->placeTransOnParent()
              && placeOnParent(frame, parent)) {
        return;
    }

    X11::getMousePosition(_ptr_x, _ptr_y);
    int head_nr = X11::getNearestHead(_ptr_x, _ptr_y);
    X11::getHeadInfoWithEdge(head_nr, _gm);

    // Collect the information which head has a fullscreen window.
    // To be conservative for now we ignore fullscreen windows on
    // the desktop or normal layer, because it might be a file
    // manager in desktop mode, for example.
    vector<bool> fsHead(X11::getNumHeads(), false);
    Workspaces::const_iterator it(Workspaces::begin()),
                              end(Workspaces::end());
    for (; it != end; ++it) {
        if ((*it)->isMapped() && (*it)->getType() == PWinObj::WO_FRAME) {
            Client *client = static_cast<Frame*>(*it)->getActiveClient();
            if (client && client->isFullscreen()
                       && client->getLayer()>LAYER_NORMAL) {
                fsHead[client->getHead()] = true;
            }
        }
    }

    // Try to place the window
    int i = head_nr;
    do {
        if (! fsHead[i]) {
            X11::getHeadInfoWithEdge(i, _gm);
            if (layout_impl(frame)) {
                return;
            }
        }
        i = (i+1)%X11::getNumHeads();
    } while (i != head_nr);

    // We failed to place the window, so put it in the top-left
    // corner but still try to avoid heads with a fullscreen window on it.
    i = head_nr;
    do {
        if (! fsHead[i]) {
            break;
        }
        i = (i+1)%X11::getNumHeads();
    } while (i != head_nr);
    X11::getHeadInfoWithEdge(i, _gm);
    frame->move(_gm.x, _gm.y);
}

void
WinLayouter::setOption(vector<string> &opt, Frame *frame)
{
    vector<string>::size_type nropts = opt.size();

    // If frame == 0 (perhaps SetPlacementOption was called by a menu entry),
    // try to get a frame.
    if (! frame) {
        if (! (frame = dynamic_cast<Frame*>(PWinObj::getFocusedPWinObj()))) {
            vector<Frame *>::const_iterator it = WindowManager::instance()->mru_begin();
            vector<Frame *>::const_iterator end = WindowManager::instance()->mru_begin();
            for (; it != end; ++it) {
                if ((*it)->isMapped()) {
                    frame = *it;
                    break;
                }
            }
            if (! frame) {
                return;
            }
        }
    }

    if (! strcasecmp(opt[0].c_str(), "switchgeometry")) {
        Frame *last = frame;
        vector<Frame*>::const_iterator it, end;

        if (nropts > 1 && Util::isTrue(opt[1])) {
            it = WindowManager::instance()->mru_begin();
            end = WindowManager::instance()->mru_end();
            for (; it != end; ++it) {
                last = *it;
                if (last != frame && last->isMapped() && ! last->isShaded()) {
                    break;
                }
            }
        } else {
            end = Frame::frame_end();
            it = find(Frame::frame_begin(), end, frame);

            do {
                if (++it == end)
                    it = Frame::frame_begin();
                last = *it;
            } while (last != frame && (!last->isMapped() || last->isShaded()));
        }

        if (it != end && last != frame) {
            int x = frame->getX();
            int y = frame->getY();
            uint w = frame->getWidth();
            uint h = frame->getHeight();

            frame->moveResize(last->getX(), last->getY(),
                              last->getWidth(), last->getHeight());
            last->moveResize(x, y, w, h);

            if (nropts > 2 && Util::isTrue(opt[2])) {
                last->raise();
                last->giveInputFocus();
            }
        }
    }
}

bool
WinLayouter::placeOnParent(Frame *wo, Window parent)
{
    PWinObj *wo_s = PWinObj::findPWinObj(parent);
    if (wo_s) {
        wo->move(wo_s->getX() + wo_s->getWidth() / 2 - wo->getWidth() / 2,
                 wo_s->getY() + wo_s->getHeight() / 2 - wo->getHeight() / 2);
        return true;
    }

    return false;
}

int WinLayouter::_ptr_x;
int WinLayouter::_ptr_y;
Geometry WinLayouter::_gm;

WinLayouter *WinLayouterFactory(std::string l) {
    Util::to_upper(l);
    const char *str = l.c_str();

    if (! strcmp(str, "SMART")) {
        return new LayouterSmart;
    }
    if (! strcmp(str, "MOUSENOTUNDER")) {
        return new LayouterMouseNotUnder;
    }
    if (! strcmp(str, "MOUSECENTERED")) {
        return new LayouterMouseCentred;
    }
    if (! strcmp(str, "MOUSETOPLEFT")) {
        return new LayouterMouseTopLeft;
    }
    if (! strncmp("TILE_", str, 5)) {
        str += 5;
        if (! strcmp("BOXED", str)) {
            return new LayouterBoxed(false);
        }
        if (! strcmp("CENTERONE", str)) {
            return new LayouterBoxed(true);
        }
        if (! strcmp("DWINDLE", str)) {
            return new LayouterDwindle;
        }
        if (! strcmp("FIBONACCI", str)) {
            return new LayouterFibonacci;
        }
        if (! strcmp("HORIZ", str)) {
            return new LayouterLayers(true);
        }
        if (! strcmp("STACKED", str)) {
            return new LayouterStacked;
        }
        if (! strcmp("TRIPLE", str)) {
            return new LayouterTriple;
        }
        if (! strcmp("VERT", str)) {
            return new LayouterLayers(false);
        }
    }

    return 0;
}
