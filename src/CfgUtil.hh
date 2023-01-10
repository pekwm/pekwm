//
// CfgUtil.hh for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CFG_UTIL_HH_
#define _PEKWM_CFG_UTIL_HH_

#include <string>

#include "CfgParser.hh"

namespace CfgUtil {

	void getThemeDir(const CfgParser::Entry* root,
			 std::string &theme_dir, std::string &theme_variant,
			 std::string &theme_path);
	void getIconDir(const CfgParser::Entry* root, std::string &icon_dir);
	void getScriptsDir(const CfgParser::Entry* root,
			   std::string &scripts_dir);

	void getFontSettings(const CfgParser::Entry* root,
			     bool &default_is_x11,
			     std::string &charset_override);

}

#endif // _PEKWM_CFG_UTIL_HH_
