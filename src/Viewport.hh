//
// Viewport.hh for pekwm
// Copyright (C)  2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _VIEWPORT_HH_
#define _VIEWPORT_HH_

#include "pekwm.hh"
#include "PWinObj.hh"

#include <list>
#include <vector>

class PScreen;

class Viewport {
public:
    Viewport(uint ws, const std::list<PWinObj*> &wo_list);
    ~Viewport(void);

    inline int getX(void) const { return _x; }
    inline int getY(void) const { return _y; }
    inline uint getWidth(void) const { return _width; }
    inline uint getHeight(void) const { return _height; }
    inline uint getCols(void) const { return _cols; }
    inline uint getRows(void) const { return _rows; }

    inline int getXMax(void) const { return _x_max; }
    inline int getYMax(void) const { return _y_max; }

    inline uint getCol(PWinObj *wo) {
        int x = wo->getX() + wo->getWidth() / 2 + _x;
        if (x < 0) // for safe unsigned cast
            x = 0;
        else if (x > _x_max)
            x = _x_max;
        return (x / _v_width);
    }
    inline uint getRow(PWinObj *wo) {
        int y = wo->getY() + wo->getHeight() / 2 + _y;
        if (y < 0) // for safe unsigned cast
            y = 0;
        else if (y > _y_max)
            y = _height;
        return (y / _v_height);
    }
    inline bool isInside(PWinObj *wo) {
        if ((wo->getRX() > 0) && (wo->getX() < signed(_v_width)) &&
                (wo->getBY() > 0) && (wo->getY() < signed(_v_height))) {
            return true;
        }
        return false;
    }

    bool move(int x, int y, bool warp = false, int wx = 0, int wy = 0);
    bool moveDirection(DirectionType dir, bool warp = true);
    void moveDrag(int x, int y);
    bool moveToWO(PWinObj *wo);
    void scroll(int x_off, int y_off);
    void gotoColRow(uint col, uint row);

    void hintsUpdate(void);
    void hintSetViewport(void);
    void hintSetGeometry(void);
    void hintSetWorkarea(void);

#ifdef HAVE_XRANDR
    void updateGeometry(void);
#endif // HAVE_XRANDR

    void reload(void);
    void makeAllInsideReal(void);
    void makeAllInsideVirtual(void);
    void makeWOInsideReal(PWinObj *wo);
    void makeWOInsideVirtual(PWinObj *wo);

private:
    PScreen *_scr;
    uint _ws;
    const std::list<PWinObj*> &_wo_list;

    int _x, _y;
    uint _v_width, _v_height;
    uint _width, _height;

    int _x_max, _y_max;
    uint _cols, _rows;
};

#endif // _VIEWPORT_HH_
