//
// ThemeUtil.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _THEME_UTIL_HH_
#define _THEME_UTIL_HH_

#include "CfgParser.hh"

#include <string>

namespace ThemeUtil {
	bool load(CfgParser &cfg, const std::string &dir,
		  const std::string &file, bool overwrite);
	void loadRequire(CfgParser &cfg, const std::string &dir,
			 const std::string &file);
};

#endif // _THEME_UTIL_HH_
