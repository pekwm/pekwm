//
// CfgUtil.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "CfgUtil.hh"
#include "Debug.hh"
#include "Util.hh"

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
			std::vector<CfgParserKey*> keys;
			keys.push_back(new CfgParserKeyPath("THEME", dir, THEME_DEFAULT));
			keys.push_back(new CfgParserKeyString("THEMEVARIANT", variant));
			files->parseKeyValues(keys.begin(), keys.end());
			std::for_each(keys.begin(), keys.end(),
				      Util::Free<CfgParserKey*>());
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
			std::string theme_file_variant = theme_file + "-" + variant;
			if (Util::isFile(theme_file_variant)) {
				theme_file = theme_file_variant;
			} else {
				P_DBG("theme variant " << variant << " does not exist");
			}
		}
	}

	void
	getIconDir(const CfgParser::Entry* root, std::string& dir)
	{
		dir = "~/.pekwm/icons/";
		Util::expandFileName(dir);

		CfgParser::Entry *files = root->findSection("FILES");
		if (files != nullptr) {
			std::vector<CfgParserKey*> keys;
			keys.push_back(new CfgParserKeyPath("ICONS", dir));
			files->parseKeyValues(keys.begin(), keys.end());
			std::for_each(keys.begin(), keys.end(),
				      Util::Free<CfgParserKey*>());
		}
	}
}
