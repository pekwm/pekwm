//
// pekwm_env.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_ENV_HH_
#define _PEKWM_ENV_HH_

#include <string>

void initEnv(bool override);
void initEnvConfig(const std::string& cfg_path, const std::string& cfg_file);

#endif // _PEKWM_ENV_HH_
