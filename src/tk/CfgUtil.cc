//
// CfgUtil.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "CfgUtil.hh"
#include "Debug.hh"
#include "Util.hh"
#include "X11.hh"

#define THEME_DEFAULT DATADIR "/pekwm/themes/default/theme"

namespace CfgUtil
{

	/**
	 * Get theme directory details from CfgParser::Entry
	 */
	void
	getThemeDir(const CfgParser::Entry* root,
		    std::string& dir, std::string& variant,
		    std::string& theme_file)
	{
		CfgParser::Entry *files = root->findSection("FILES");
		if (files != nullptr) {
			CfgParserKeys keys;
			keys.add_path("THEME", dir, THEME_DEFAULT);
			keys.add_string("THEMEVARIANT", variant);
			files->parseKeyValues(keys.begin(), keys.end());
			keys.clear();
		} else {
			dir = THEME_DEFAULT;
			variant = "";
		}

		std::string norm_dir(dir);
		if (dir.size() && dir.at(dir.size() - 1) == '/') {
			norm_dir.erase(norm_dir.end() - 1);
		}

		theme_file = norm_dir + "/theme";
		if (! variant.empty()) {
			lookupThemeVariant(theme_file, variant);
			std::string theme_file_variant =
				theme_file + "-" + variant;
			if (Util::isFile(theme_file_variant)) {
				theme_file = theme_file_variant;
			} else {
				P_DBG("theme variant " << variant
				      << " does not exist");
			}
		}
	}

	bool
	lookupThemeVariant(const std::string &theme_file, std::string &variant)
	{
		if (variant != "auto") {
			return false;
		}

		if (! X11::getString(X11::getRoot(), PEKWM_THEME_VARIANT,
				     variant)) {
			variant = "light";
		}

		std::string theme_file_variant = theme_file + "-" + variant;
		if (! Util::isFile(theme_file_variant)
		    && (variant == "dusk" || variant == "dawn")) {
			variant = "dark";
		}
		return true;
	}

	void
	getIconDir(const CfgParser::Entry* root, std::string& dir)
	{
		dir = Util::getConfigDir() + "/icons/";
		Util::expandFileName(dir);

		CfgParser::Entry *files = root->findSection("FILES");
		if (files != nullptr) {
			CfgParserKeys keys;
			keys.add_path("ICONS", dir);
			files->parseKeyValues(keys.begin(), keys.end());
			keys.clear();
		}
	}

	/**
	 * Get script path from CfgParser::Entry (root of ~/.pekwm/config)
	 */
	void
	getScriptsDir(const CfgParser::Entry* root, std::string &dir)
	{
		dir = getDefaultScriptsDir();
		Util::expandFileName(dir);

		CfgParser::Entry *files = root->findSection("FILES");
		if (files != nullptr) {
			CfgParserKeys keys;
			keys.add_path("SCRIPTS", dir);
			files->parseKeyValues(keys.begin(), keys.end());
			keys.clear();
		}
	}

	std::string
	getDefaultScriptsDir()
	{
		std::string dir = Util::getConfigDir() + "/scripts";
		Util::expandFileName(dir);
		return dir;
	}
	/**
	 * Return options used to initialize FontHandler
	 */
	void
	getFontSettings(const CfgParser::Entry* root,
			bool &default_is_x11,
			std::string &charset_override)
	{
		default_is_x11 = false;
		CfgParser::Entry *screen = root->findSection("SCREEN");
		if (screen != nullptr) {
			CfgParserKeys keys;
			keys.add_bool("FONTDEFAULTX11", default_is_x11, false);
			keys.add_string("FONTCHARSETOVERRIDE",
					charset_override);
			screen->parseKeyValues(keys.begin(), keys.end());
			keys.clear();
		}
	}

}
