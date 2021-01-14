//
// pekwm.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"

static bool s_is_startup = true;

namespace pekwm
{
    bool isStartup()
    {
        return s_is_startup;
    }

    void setIsStartup(bool is_startup)
    {
        s_is_startup = is_startup;
    }
}
