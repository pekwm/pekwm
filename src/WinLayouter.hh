//
// WinLayouter.hh for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WIN_LAYOUTER_HH_
#define _WIN_LAYOUTER_HH_

#include <string>
#include <vector>

class Frame;

class WinLayouter {
public:
    WinLayouter(bool tiling) : _tiling(tiling) {}
    virtual ~WinLayouter() {}

    bool isTiling(void) const { return _tiling; }

    void layout(Frame *f, Window parent);
    virtual void setOption(std::vector<std::string> &, Frame *);

protected:
    // temp. variables that get filled in by layout() (if !_tiling).
    static int _ptr_x, _ptr_y; // mouse pointer coordinates
    static Geometry _gm; // geometry of the head

private:
    bool placeOnParent(Frame *f, Window parent);
    bool _tiling;

    virtual bool layout_impl(Frame *f)=0;
};

WinLayouter *WinLayouterFactory(std::string name);

#endif // _WIN_LAYOUTER_HH_
