//
// TkGlobals.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TK_GLOBALS_HH_
#define _TK_GLOBALS_HH_

#include <string>

namespace pekwm
{
	const std::string& configScriptPath();
	void setConfigScriptPath(const std::string& config_script_path);
}

#endif // _TK_GLOBALS_HH_
