//
// WinLayouter.cc for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Workspaces.hh"
#include "x11.hh"

PWinObj*
Workspaces::isEmptySpace(int x, int y, const PWinObj* wo)
{
    if (! wo) {
        return 0;
    }

    // say that it's placed, now check if we are wrong!
    const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
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
            if (client && client->isMaximizedVert() && client->isMaximizedHorz()) {
                continue;
            }
        }

        // Check if we are "intruding" on some other window's place
        if (((*it)->getX() < signed(x + wo->getWidth())) &&
            (signed((*it)->getX() + (*it)->getWidth()) > x) &&
            ((*it)->getY() < signed(y + wo->getHeight())) &&
            (signed((*it)->getY() + (*it)->getHeight()) > y)) {
            return (*it);
        }
    }

    return 0; // we passed the test, no frames in the way
}

bool
Workspaces::placeWo(PWinObj *wo, uint head_num, Window parent)
{
    bool placed = false;
    Geometry head;
    X11::getHeadInfoWithEdge(head_num, head);

    vector<uint>::const_iterator it(Config::instance()->getPlacementModelBegin());
    for (; !placed && it != Config::instance()->getPlacementModelEnd(); ++it) {
        switch (static_cast<enum PlacementModel>(*it)) {
        case PLACE_SMART:
            placed = placeSmart(wo, head);
            break;
        case PLACE_MOUSE_NOT_UNDER:
            placed = placeMouseNotUnder(wo, head);
            break;
        case PLACE_MOUSE_CENTERED:
            placed = placeMouseCentered(wo);
            break;
        case PLACE_MOUSE_TOP_LEFT:
            placed = placeMouseTopLeft(wo);
            break;
        case PLACE_CENTERED_ON_PARENT:
            placed = placeCenteredOnParent(wo, parent);
            break;
        default:
            placed = false;
            break;
        }
    }

    return placed;
}

//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
bool
Workspaces::placeSmart(PWinObj* wo, const Geometry &head)
{
    PWinObj *wo_e;
    bool placed = false;

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

    start_x = (Config::instance()->getPlacementLtR())
              ? (head.x)
              : (head.x + head.width - wo_width);
    start_y = (Config::instance()->getPlacementTtB())
              ? (head.y)
              : (head.y + head.height - wo_height);

    if (Config::instance()->getPlacementRow()) { // row placement
        test_y = start_y;
        while (! placed && (Config::instance()->getPlacementTtB()
                           ? ((test_y + wo_height) <= (head.y + head.height))
                           : (test_y >= head.y))) {
            test_x = start_x;
            while (! placed && (Config::instance()->getPlacementLtR()
                               ? ((test_x + wo_width) <= (head.x + head.width))
                               : (test_x >= head.x))) {
                // see if we can place the window here
                if ((wo_e = isEmptySpace(test_x, test_y, wo))) {
                    placed = false;
                    test_x = Config::instance()->getPlacementLtR()
                             ? (wo_e->getX() + wo_e->getWidth()) : (wo_e->getX() - wo_width);
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
                           ? ((test_x + wo_width) <= (head.x + head.width))
                           : (test_x >= head.x))) {
            test_y = start_y;
            while (! placed && (Config::instance()->getPlacementTtB()
                               ? ((test_y + wo_height) <= (head.y + head.height))
                               : (test_y >= head.y))) {
                // see if we can place the window here
                if ((wo_e = isEmptySpace(test_x, test_y, wo))) {
                    placed = false;
                    test_y = Config::instance()->getPlacementTtB()
                             ? (wo_e->getY() + wo_e->getHeight()) : (wo_e->getY() - wo_height);
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

//! @brief Places the wo in a corner of the screen not under the pointer
bool
Workspaces::placeMouseNotUnder(PWinObj *wo, const Geometry &head)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    // compensate for head offset
    mouse_x -= head.x;
    mouse_y -= head.y;

    // divide the screen into four rectangles using the pointer as divider
    if ((wo->getWidth() < unsigned(mouse_x)) && (wo->getHeight() < head.height)) {
        wo->move(head.x, head.y);
        return true;
    }

    if ((wo->getWidth() < head.width) && (wo->getHeight() < unsigned(mouse_y))) {
        wo->move(head.x + head.width - wo->getWidth(), head.y);
        return true;
    }

    if ((wo->getWidth() < (head.width - mouse_x)) && (wo->getHeight() < head.height)) {
        wo->move(head.x + head.width - wo->getWidth(), head.y + head.height - wo->getHeight());
        return true;
    }

    if ((wo->getWidth() < head.width) && (wo->getHeight() < (head.height - mouse_y))) {
        wo->move(head.x, head.y + head.height - wo->getHeight());
        return true;
    }

    return false;
}

//! @brief Places the client centered under the mouse
bool
Workspaces::placeMouseCentered(PWinObj *wo)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x - (wo->getWidth() / 2), mouse_y - (wo->getHeight() / 2),
                wo->getWidth(), wo->getHeight());

    // make sure it's within the screen's border
    X11::placeInsideScreen(gm);

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places the client like the menu gets placed
bool
Workspaces::placeMouseTopLeft(PWinObj *wo)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x, mouse_y, wo->getWidth(), wo->getHeight());
    X11::placeInsideScreen(gm); // make sure it's within the screen's border

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places centerd on the window parent
bool
Workspaces::placeCenteredOnParent(PWinObj *wo, Window parent)
{
    if (parent == None) {
        return false;
    }

    PWinObj *wo_s = PWinObj::findPWinObj(parent);
    if (wo_s) {
        wo->move(wo_s->getX() + wo_s->getWidth() / 2 - wo->getWidth() / 2,
                 wo_s->getY() + wo_s->getHeight() / 2 - wo->getHeight() / 2);
        return true;
    }

    return false;
}

