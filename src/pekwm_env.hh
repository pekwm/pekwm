//
// pekwm_env.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_ENV_HH_
#define _PEKWM_ENV_HH_

#include "config.h"
#include "Compat.hh"

static void
initEnvSingle(bool override, const char* key, const char* val)
{
	const char *env_val = getenv(key);
	if (override || (env_val == nullptr || env_val[0] == '\0')) {
		setenv(key, val, 1);
	}
}

/**
 * Setup environment variables pointing to pekwm environment.
 *
 * @param override Override existing environment variables if true.
 */
static void
initEnv(bool override)
{
	initEnvSingle(override, "PEKWM_ETC_PATH", SYSCONFDIR);
	initEnvSingle(override, "PEKWM_SCRIPT_PATH", DATADIR "/pekwm/scripts");
	initEnvSingle(override, "PEKWM_THEME_PATH", DATADIR "/pekwm/themes");
}

/**
 * Setup environment variables pointing to pekwm configuration file.
 */
static void
initEnvConfig(const std::string& cfg_path, const std::string& cfg_file)
{
	initEnvSingle(true, "PEKWM_CONFIG_PATH", cfg_path.c_str());
	initEnvSingle(true, "PEKWM_CONFIG_FILE", cfg_file.c_str());
}

#endif // _PEKWM_ENV_HH_
