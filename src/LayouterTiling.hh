//
// LayouterTiling.hh for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _LAYOUTERTILING_HH_
#define _LAYOUTERTILING_HH_

#include "WinLayouter.hh"

class LayouterTiling : public WinLayouter {
public:
    LayouterTiling(void) : WinLayouter(true) { }
    virtual ~LayouterTiling() { }

    virtual void setOption(vector<std::string> &opts, Frame *frame) {
        default_setOption(opts, frame);
    }

protected:
    bool setupFrames(Geometry &gm);
    void default_setOption(vector<std::string> &opts, Frame *frame);
    vector<Frame *> _frames;
};


/**
 *  Boxed layout implementation
 *
 * With centre == false:
 *  -------------
 *  |   |   |   |
 *  | 1 | 2 | 3 |
 *  |-----------|
 *  |   |   |   |
 *  | 4 | 5 | 6 |
 *  -------------
 *
 * With centre == true:
 *  ----------------
 *  | 2  | 3  |  4 |
 *  |   +------+   |
 *  |---|  1   |---|
 *  |   +------+   |
 *  | 5  | 6  |  7 |
 *  ----------------
 */
class LayouterBoxed : public LayouterTiling {
public:
    LayouterBoxed(bool centre) : LayouterTiling(), _centre(centre) {}
    virtual ~LayouterBoxed() {}


private:
    bool _centre;

    virtual bool layout_impl(Frame *frame);
};

class LayouterDwindle : public LayouterTiling {
public:
    LayouterDwindle() : LayouterTiling() {}
    ~LayouterDwindle() {}

private:
    virtual bool layout_impl(Frame *frame);
};

class LayouterFibonacci : public LayouterTiling {
public:
    LayouterFibonacci() : LayouterTiling() {}
    ~LayouterFibonacci() {}

private:
    virtual bool layout_impl(Frame *frame);
    static const double Fib[];
};

/**
 *  horiz == false:
 *  -----------------
 *  |       1       |
 *  |---------------|
 *  |       2       |
 *  |---------------|
 *  |       3       |
 *  |---------------|
 *  |       4       |
 *  -----------------
 *
 *  horiz == true:
 *  -----------------
 *  |   |   |   |   |
 *  | 1 | 2 | 3 | 4 |
 *  |   |   |   |   |
 *  |   |   |   |   |
 *  -----------------
 */
class LayouterLayers : public LayouterTiling {
public:
    LayouterLayers(bool horiz) : LayouterTiling(), _horiz(horiz) {}
    ~LayouterLayers() {}

private:
    bool _horiz;

    virtual bool layout_impl(Frame *frame);
};

/**
 *  One big window on the left and all other stacked on the right.
 *
 *  ----------------
 *  |         |  2 |
 *  |         +----+
 *  |    1    |  3 |
 *  |         +----+
 *  |         |  4 |
 *  ----------------
 */
class LayouterStacked : public LayouterTiling {
public:
    LayouterStacked() : LayouterTiling() {}
    ~LayouterStacked() {}

private:
    virtual bool layout_impl(Frame *frame);
};

/**
 *  Three area layout implementation.
 *
 *  ----------------
 *  |              |
 *  |      1       |
 *  |              |
 *  |______________|   <- The bar is adjustable with a % option, default is 20.
 *  |  2   |   3   |
 *  ----------------
 */
class LayouterTriple: public LayouterTiling {
public:
    LayouterTriple() : LayouterTiling(), _height(20) {}
    ~LayouterTriple() {}

    virtual void setOption(vector<std::string> &opts, Frame *frame);

private:
    uint _height;

    virtual bool layout_impl(Frame *frame);
};

#endif // _LAYOUTERTILING_HH_
