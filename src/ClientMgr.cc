//
// ClientMgr.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "AutoProperties.hh"
#include "ClientMgr.hh"
#include "Client.hh"
#include "Frame.hh"

bool ClientMgr::_allow_grouping = true;

/**
 * Tries to find a Frame to autogroup with. This is called recursively
 * if workspace specific matching is on to avoid conflicts with the
 * global property.
 */
Frame*
ClientMgr::findGroup(AutoProperty *property)
{
    if (! _allow_grouping) {
        return nullptr;
    }

    Frame *frame = nullptr;
    if (property->group_global && property->isMask(AP_WORKSPACE)) {
        bool group_global_orig = property->group_global;
        property->group_global= false;
        frame = findGroup(property);
        property->group_global= group_global_orig;
    }

    if (! frame) {
        frame = findGroupMatch(property);
    }

    return frame;
}

/**
 * Match Frame against autoproperty.
 */
bool
ClientMgr::findGroupMatchProperty(Frame *frame, AutoProperty *property)
{
    if ((property->group_global
         || (property->isMask(AP_WORKSPACE)
             ? (frame->getWorkspace() == property->workspace
                && ! frame->isIconified())
             : frame->isMapped()))
        && (property->group_size == 0
            || signed(frame->size()) < property->group_size)
        && (((frame->getClassHint()->group.size() > 0)
             ? (frame->getClassHint()->group == property->group_name) : false)
            || AutoProperties::matchAutoClass(*frame->getClassHint(),
                                              property))) {
        return true;
    }
    return false;
}

/**
 * Do matching against Frames searching for a suitable Frame for
 * grouping.
 */
Frame*
ClientMgr::findGroupMatch(AutoProperty *property)
{
    Frame *frame = nullptr;

    // Matching against the focused window first if requested
    if (property->group_focused_first
        && PWinObj::isFocusedPWinObj(PWinObj::WO_CLIENT)) {
        Frame *fo_frame =
            static_cast<Frame*>(PWinObj::getFocusedPWinObj()->getParent());
        if (findGroupMatchProperty(fo_frame, property)) {
            frame = fo_frame;
        }
    }

    // Moving on to the rest of the frames.
    if (! frame) {
        auto it(Frame::frame_begin());
        for (; it != Frame::frame_end(); ++it) {
            if (findGroupMatchProperty(*it, property)) {
                frame = *it;
                break;
            }
        }
    }

    return frame;
}
