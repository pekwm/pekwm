//
// LayouterTiling.cc for pekwm
// Copyright © 2012-2013 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "LayouterTiling.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Util.hh"
#include "WindowManager.hh"
#include "Workspaces.hh"
#include "x11.hh"

using std::string;
typedef vector<Frame *>::size_type vecsize_t;

static void
collectTilingFrames(vector<Frame *> &vec, Geometry &gm) {
    Client *client;
    Frame *frame;

    vector<PWinObj*>::const_iterator it = Workspaces::begin();
    vector<PWinObj*>::const_iterator end = Workspaces::end();
    for (; it != end; ++it) {
        if (! (frame = dynamic_cast<Frame*>(*it)))
            continue;
        if (frame->isMapped() && ! frame->isFullscreen() && ! frame->isSticky()) {
            client = frame->getActiveClient();
            if (client && ! client->isCfgDeny(CFG_DENY_TILING)
                       && client->getWinType() == WINDOW_TYPE_NORMAL)
            {
                vec.push_back(frame);
            }
        }
    }

    X11::getHeadInfoWithEdge(X11::getCurrHead(), gm);
    if (vec.size() == 1) {
        vec.back()->checkMoveResize(gm.x, gm.y, gm.width, gm.height);
        vec.clear();
    }
}

bool
LayouterTiling::setupFrames(Geometry &gm)
{
    vector<Frame *> vec, vec2;
    vecsize_t i,j;

    collectTilingFrames(vec, gm);
    if (vec.empty())
        return false;

    // vec2 := _frames \cap vec
    for (i=0; i<_frames.size(); ++i) {
        for (j=0; j<vec.size(); ++j) {
            if (vec[j] == _frames[i]) {
                vec2.push_back(_frames[i]);
                break;
            }
        }
    }

    _frames = vec2;

    vecsize_t old_size = _frames.size();
    bool found;
    for (i=0; i<vec.size(); ++i) {
        found = false;
        for (j=0; j<old_size; ++j) {
            if (vec[i] == _frames[j]) {
                found = true;
                break;
            }
        }
        if (! found) {
            _frames.push_back(vec[i]);
        }
    }
    return true;
}

void
LayouterTiling::default_setOption(vector<string> &opts, Frame *frame)
{
    if (_frames.empty())
        return;

    vector<string>::size_type nropts = opts.size();
    const char *str = opts[0].c_str();

    if (! strcasecmp(str, "cycle")) {
        frame = _frames.front();
        for (uint i=1; i < _frames.size(); ++i) {
            _frames[i-1] = _frames[i];
        }
        _frames.back() = frame;
        layout(0, false);
        return;
    }

    if (! frame)
        frame = *WindowManager::instance()->mru_begin();

    if (frame) {
        const vector<Frame *>::iterator end = _frames.end();
        const vector<Frame *>::iterator it = find(_frames.begin(), end, frame);
        vector<Frame *>::iterator it2;

        if (it == end)
            return;

        if (! strcasecmp(str, "switch")) {
            frame = _frames.front();
            _frames.front() = *it;
            *it = frame;
        } else if (! strcasecmp(str, "switchgeometry")) {
            vector<Frame*>::const_iterator fit, fend;
            if (nropts > 1 && Util::isTrue(opts[1])) {
                fit = WindowManager::instance()->mru_begin();
                fend = WindowManager::instance()->mru_end();
                for (; fit != fend; ++fit) {
                    if ((*fit)->isMapped() && *fit != frame) {
                        it2 = find(_frames.begin(), _frames.end(), *fit);
                        if (it2 != end) {
                            break;
                        }
                    }
                }
            } else {
                fend = Frame::frame_end();
                fit = find(Frame::frame_begin(), fend, frame);
                for (;;) {
                    if (++fit == fend)
                        fit = Frame::frame_begin();
                    if (*fit == frame) {
                        // Reaching frame again means that we have come
                        // full circle and should break out of the loop.
                        fit = fend; // signal our failure to find a 2nd frame
                        break;
                    }
                    if ((*fit)->isMapped()) {
                        it2 = find(_frames.begin(), _frames.end(), *fit);
                        if (it2 != end) {
                            break;
                        }
                    }
                }
            }
            if (fit != fend) {
                *it = *it2;
                *it2 = frame;
                if (nropts > 2 && Util::isTrue(opts[2])) {
                    (*it2)->raise();
                    (*it2)->giveInputFocus();
                }
            }
        }

        layout(0, false);
    }
}

bool
LayouterBoxed::layout_impl(Frame *)
{
    Geometry gm;
    uint cols;
    vecsize_t i;
    int x, y;

    if (! setupFrames(gm)) {
        return true;
    }

    if (_centre) {
        cols = _frames.size()/2;
        x = gm.x + gm.width/4;
        y = gm.y + gm.height/4;
        _frames[0]->checkMoveResize(x, y, gm.width/2, gm.height/2);
        i = 1;
    } else {
        cols = (_frames.size()+1)/2;
        i = 0;
    }
    gm.width /= cols;
    gm.height /= 2;
    x = gm.x;
    y = gm.y;

    for (;i < _frames.size(); ++i) {
        _frames[i]->checkMoveResize(x, y, gm.width, gm.height);
        if (gm.y != y) {
            y = gm.y;
            x += gm.width;
        } else {
            y = gm.height;
        }
    }

    _frames[0]->raise();
    return true;
}

bool
LayouterDwindle::layout_impl(Frame *)
{
    Geometry gm;
    vecsize_t i;
    bool horiz=false;

    if (! setupFrames(gm)) {
        return true;
    }

    gm.width /= 2;
    _frames[0]->checkMoveResize(gm.x, gm.y, gm.width, gm.height);

    for (i=1; i < _frames.size()-1; ++i) {
        if (horiz) {
            gm.width /= 2;
            gm.y += gm.height;
        } else {
            gm.height /= 2;
            gm.x += gm.width;
        }

        _frames[i]->checkMoveResize(gm.x, gm.y, gm.width, gm.height);
        horiz = !horiz;
    }

    if (horiz) {
        gm.y += gm.height;
    } else {
        gm.x += gm.width;
    }
    _frames.back()->checkMoveResize(gm.x, gm.y, gm.width, gm.height);
    return true;
}

bool
LayouterFibonacci::layout_impl(Frame *)
{
    Geometry gm;
    uint NrFib = 11; // max. index for Fib (12-1)
    uint oldw, oldh;
    uint w=0, h=0, x = 0, y = 0;

    if (! setupFrames(gm)) {
        return true;
    }

    oldw = gm.width;
    oldh = gm.height;

    if (_frames.size() < NrFib)
        NrFib = _frames.size();

    w = oldw * Fib[NrFib]/(Fib[NrFib] + Fib[NrFib-1]);
    h = oldh;
    _frames[0]->checkMoveResize(x, y, w, h);
    NrFib--;
    vecsize_t i;
    for (i = 0; i < NrFib; ++i) {
        switch (i%4) {
        case 0:
            x += w;
            w = oldw - w;
            oldh = h;
            h *= Fib[NrFib-i]/(Fib[NrFib-i] + Fib[NrFib-1-i]);
            break;
        case 1:
            y += h;
            h = oldh - h;
            oldw = w;
            w *= Fib[NrFib-i]/(Fib[NrFib-i] + Fib[NrFib-1-i]);
            x += oldw - w;
            break;
        case 2:
            x -= oldw - w;
            w = oldw - w;
            oldh = h;
            h *= Fib[NrFib-i]/(Fib[NrFib-i] + Fib[NrFib-1-i]);
            y += oldh - h;
            break;
        case 3:
            y -= oldh - h;
            h = oldh - h;
            oldw = w;
            w *= Fib[NrFib-i]/(Fib[NrFib-i] + Fib[NrFib-1-i]);
            break;
        }
        _frames[i+1]->checkMoveResize(x, y, w, h);
    }

    for (; i < _frames.size(); ++i) {
        _frames[i]->checkMoveResize(x, y, w, h);
    }
    return true;
}

const double LayouterFibonacci::Fib[] = { 0,1,1,2,3,5,8,13,21,34,55,89 };

bool
LayouterLayers::layout_impl(Frame *)
{
    Geometry gm;
    uint nr_wins, delta_x, delta_y, width, height;
    int x, y;

    if (! setupFrames(gm)) {
        return true;
    }

    nr_wins = _frames.size();
    delta_x = _horiz?gm.width/nr_wins:0;
    delta_y = _horiz?0:gm.height/nr_wins;
    if (! delta_x && ! delta_y) {
        if (_horiz)
            delta_x = 10;
        else
            delta_y = 10;
    }

    x = gm.x;
    y = gm.y;
    height = gm.height;
    width = gm.width;

    if (_horiz) {
        width /= nr_wins;
        if (! width)
            width = 10;
    } else {
        height /= nr_wins;
        if (! height)
            height = 10;
    }

    for (vecsize_t i=0; i<_frames.size(); ++i) {
        _frames[i]->checkMoveResize(x, y, width, height);
        x += delta_x;
        y += delta_y;
        if (y > 0 && static_cast<unsigned>(y) > gm.height) {
            y = gm.y;
        }
        if (x > 0 && static_cast<unsigned>(x) > gm.width) {
            x = gm.x;
        }
    }
    return true;
}

bool
LayouterStacked::layout_impl(Frame *)
{
    Geometry gm;
    uint x, y, h;

    if (! setupFrames(gm)) {
        return true;
    }

    x = (gm.width*2)/3;
    y = gm.y;
    _frames[0]->checkMoveResize(gm.x, gm.y, x, gm.height);
    gm.x += x;
    gm.width -= x;
    h = gm.height / (_frames.size()-1);
    if (!h)
        h = 10;

    for (vecsize_t i=1; i<_frames.size(); ++i) {
        _frames[i]->checkMoveResize(gm.x, y, gm.width, h);
        if (y+h > gm.height)
            y = gm.y;
        y += h;
    }
    return true;
}

bool
LayouterTriple::layout_impl(Frame *)
{
    Geometry gm;
    uint height;

    if (! setupFrames(gm))
        return true;

    height = gm.height - int(gm.height * _height/100.0);

    _frames[0]->checkMoveResize(gm.x, gm.y, gm.width, height);

    gm.height -= int(gm.height* (100.0-_height)/100.0);

    if (_frames.size() == 2) {
        _frames[1]->checkMoveResize(gm.x, height, gm.width, gm.height);
        return true;
    }

    bool left = true;
    for (vecsize_t i=1; i<_frames.size(); ++i, left = !left) {
        if (left) {
            _frames[i]->checkMoveResize(gm.x, height, gm.y+gm.width/2, gm.height);
        } else {
            _frames[i]->checkMoveResize(gm.x+gm.width/2, height, gm.y+gm.width/2, gm.height);
        }
    }
    return true;
}

void
LayouterTriple::setOption(vector<std::string> &opts, Frame *frame)
{
    if (2 == opts.size()) {
        int height=0;
        if (! strcasecmp(opts[0].c_str(), "abs")) {
            height = atoi(opts[1].c_str());
        } else if (! strcasecmp(opts[0].c_str(), "rel")) {
            height = atoi(opts[1].c_str());
            height += _height;
        }
        if (height > 5 && height < 100)
            _height = static_cast<uint>(height);
    } else {
        default_setOption(opts, frame);
    }
}
