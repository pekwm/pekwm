//
// ThemeUtil.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "ColorPalette.hh"
#include "ThemeUtil.hh"
#include "TkGlobals.hh"

#include <map>

extern "C" {
#include <assert.h>
}

namespace ThemeUtil {

static void
setCfgParserOptions(CfgParser &cfg, const std::string &dir, bool end_early)
{
	cfg.getOpt().setCommandPath(pekwm::configScriptPath());
	cfg.getOpt().setEndEarlyKey(end_early ? "REQUIRE" : "");
	cfg.getOpt().setRegisterXResource(true);
	cfg.setVar("THEME_DIR", dir);

}

static bool
loadPalette(ColorPalette::Mode mode, ColorPalette::BaseColor base_color,
	    uint intensity, float brightness, const std::string &name,
	    std::map<std::string, std::string> &vars)
{
	std::vector<std::string> colors;
	if (! getColors(mode, base_color, intensity, brightness, colors)) {
		return false;
	}

	std::vector<std::string>::iterator it(colors.begin());
	for (int i = 0; it != colors.end(); ++it, i++) {
		std::string key = name + std::to_string(i) + "_"
			+ std::to_string(intensity);
		vars[key] = *it;
	}
	return true;
}

static void
loadPalette(CfgParser::Entry *section,
	    std::map<std::string, std::string> &vars)
{
	if (! section) {
		return;
	}

	std::string mode_str, base_str;
	float brightness;

	CfgParserKeys keys;
	keys.add_string("MODE", mode_str);
	keys.add_string("BASE", base_str);
	keys.add_numeric<float>("BRIGHTNESS", brightness, 100.0, 0.0);
	section->parseKeyValues(keys.begin(), keys.end());
	brightness /= 100.0;

	bool err = false;
	ColorPalette::Mode mode = ColorPalette::modeFromString(mode_str);
	ColorPalette::BaseColor base_color =
		ColorPalette::baseColorFromString(base_str);
	for (uint i = 0; i <= ColorPalette::MAX_INTENSITY; i++) {
		if (! loadPalette(mode, base_color, i, brightness,
				  section->getValue(), vars)) {
			err = true;
		}
	}

	if (err) {
		USER_WARN("failed to get colors for Palette "
			  << section->getValue() << ". Mode: " << mode_str
			  << " Base: " << base_str);
	}
}

bool
load(CfgParser &cfg, const std::string &dir, const std::string &file,
     bool overwrite)
{
	setCfgParserOptions(cfg, dir, true);
	if (cfg.parse(file, CfgParserSource::SOURCE_FILE, overwrite)) {
		loadRequire(cfg, dir, file);
		return true;
	}
	return false;
}

/**
 * Load template quirks and palette.
 */
void
loadRequire(CfgParser &cfg, const std::string &dir, const std::string &file)
{
	if (! cfg.isEndEarly()) {
		return;
	}

	CfgParser::Entry *section = cfg.getEntryRoot()->findSection("REQUIRE");
	assert(section); // was ended early, section must be present

	// Check if Templates are enabled and setup palette colors
	CfgParserKeys keys;
	bool value_templates;
	keys.add_bool("TEMPLATES", value_templates, false);
	section->parseKeyValues(keys.begin(), keys.end());

	std::map<std::string, std::string> vars;
	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		if (*(*it) == "PALETTE") {
			loadPalette((*it)->getSection(), vars);
		}
	}

	// Re-load configuration, potentially with templates enabled, and no
	// early end on Require
	cfg.clear(true);
	setCfgParserOptions(cfg, dir, false);
	std::map<std::string, std::string>::iterator var_it(vars.begin());
	for (; var_it != vars.end(); ++var_it) {
		cfg.setVar(var_it->first, var_it->second);
	}
	cfg.parse(file, CfgParserSource::SOURCE_FILE, value_templates);
}

};
