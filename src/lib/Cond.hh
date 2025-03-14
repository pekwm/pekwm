//
// Cond.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_COND_HH_
#define _PEKWM_COND_HH_

#include <string>

namespace Cond {
	bool eval(const std::string& cond);
};

#endif // _PEKWM_COND_HH_
