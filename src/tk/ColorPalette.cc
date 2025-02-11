//
// ColorPalette.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "ColorPalette.hh"
#include "Util.hh"

#include <algorithm>

extern "C" {
#include <assert.h>
#include <stdio.h>
}

namespace ColorPalette {

static const int COLORS_PER_BASE = 5;

const char *COLORS[] = {
	"#fbd3d0", "#f57a6d", "#ef1c26", "#b81117", "#8f0307", // Red
	"#e0cde3", "#be7cb4", "#a2218e", "#820e6e", "#68005c", // RedPurple
	"#b8add5", "#7e6aae", "#592f94", "#491d75", "#37075c", // Purple
	"#8b92c7", "#5764af", "#21409a", "#162e7c", "#081c63", // BluePurple
	"#aec4e4", "#5c89c5", "#0465b1", "#004e8c", "#013c73", // Blue
	"#bce4e7", "#56c3c5", "#00adae", "#018a89", "#006d6f", // BlueGreen
	"#bfdfd0", "#64c297", "#03a55e", "#00864b", "#006c3c", // Green
	"#e2ecd6", "#add68b", "#71bd44", "#569835", "#40782a", // YellowGreen
	"#fefac9", "#fcf583", "#fef102", "#cbbe01", "#a29600", // Yellow
	"#ffe3cb", "#fcc777", "#f8a61b", "#c28211", "#9b6702", // YellowOrange
	"#fbdcc9", "#f4a973", "#f58225", "#c26819", "#9a4f06", // Orange
	"#fccfc1", "#f48d74", "#ed403d", "#bb312d", "#972d1c"  // RedOrange
};
static const int COLORS_SIZE = sizeof(COLORS)/sizeof(COLORS[0]);

static Util::StringTo<Mode> mode_map[] =
	{{"SINGLE", PALETTE_SINGLE},
	 {"COMPLEMENTARY", PALETTE_COMPLEMENTARY},
	 {"TRIAD", PALETTE_TRIAD},
	 {"ANALOGOUS", PALETTE_ANALOGOUS},
	 {"SPLIT", PALETTE_SPLIT},
	 {"TETRAD", PALETTE_TETRAD},
	 {"SQUARE", PALETTE_SQUARE},
	 {nullptr, PALETTE_NO}};

static int mode_steps_single[] = { 0 };
static int mode_steps_complementary[] = { 6, 0 };
static int mode_steps_triad[] = { 4, 4, 0 };
static int mode_steps_analogous[] = { 1, 1, 0 };
static int mode_steps_split[] = { 5, 2, 0 };
static int mode_steps_tetrad[] = { 4, 2, 4, 0 };
static int mode_steps_square[] = { 3, 3, 3, 0 };
static int mode_steps_no[] = { 0 };
static int *mode_steps[] =
	{mode_steps_single,
	 mode_steps_complementary,
	 mode_steps_triad,
	 mode_steps_analogous,
	 mode_steps_split,
	 mode_steps_tetrad,
	 mode_steps_square,
	 mode_steps_no};

static Util::StringTo<BaseColor> base_color_map[] = 
	{{"RED", BASE_COLOR_RED},
	 {"REDPURPLE", BASE_COLOR_RED_PURPLE},
	 {"PURPLE", BASE_COLOR_PURPLE},
	 {"BLUEPURPLE", BASE_COLOR_BLUE_PURPLE},
	 {"BLUE", BASE_COLOR_BLUE},
	 {"BLUEGREEN", BASE_COLOR_BLUE_GREEN},
	 {"GREEN", BASE_COLOR_GREEN},
	 {"YELLOWGREEN", BASE_COLOR_YELLOW_GREEN},
	 {"YELLOW", BASE_COLOR_YELLOW},
	 {"YELLOWORANGE", BASE_COLOR_YELLOW_ORANGE},
	 {"ORANGE", BASE_COLOR_ORANGE},
	 {"REDORANGE", BASE_COLOR_RED_ORANGE},
	 {nullptr, BASE_COLOR_NO}};

Mode
modeFromString(const std::string &mode_str)
{
	return Util::StringToGet(mode_map, mode_str);
}

BaseColor
baseColorFromString(const std::string &base_str)
{
	return Util::StringToGet(base_color_map, base_str);
}

static void
getSteps(Mode mode, std::vector<int> &steps)
{
	if (mode >= 0 && mode < PALETTE_NO) {
		for (int i = 0; mode_steps[mode][i]; i++) {
			steps.push_back(mode_steps[mode][i]);
		}
	}
}

static uint
fromHex(const char *c_hex)
{
	char *endptr;
	char buf[3] = { c_hex[0], c_hex[1], '\0' };
	return strtol(buf, &endptr, 16);
}

static uint
toColorVal(float fval)
{
	uint val = static_cast<int>(fval);
	return std::min(val, 255U);
}

/**
 * Get single color from color map, optionally adjusting its brightess
 */
static std::string
getColor(uint idx, float brightness)
{
	assert(idx < sizeof(COLORS)/sizeof(COLORS[0]));
	if (brightness == 1.0) {
		return COLORS[idx];
	}

	const char *c_color = COLORS[idx];
	float r = fromHex(c_color + 1) * brightness;
	float g = fromHex(c_color + 3) * brightness;
	float b = fromHex(c_color + 5) * brightness;
	char buf[8];
	snprintf(buf, sizeof(buf), "#%02x%02x%02x",
		 toColorVal(r), toColorVal(g), toColorVal(b));
	return buf;
}

/**
 * Get colors as XColor for the given mode and base color. Caller is responsible
 * for calling X11::returnColor on the returned colors.
 */
bool
getColors(Mode mode, BaseColor base, uint intensity, float brightness,
	  std::vector<XColor*> colors)
{
	std::vector<std::string> str_colors;
	if (! getColors(mode, base, intensity, brightness, str_colors)) {
		return false;
	}

	std::vector<std::string>::iterator it(str_colors.begin());
	for (; it != str_colors.end(); ++it) {
		colors.push_back(X11::getColor(*it));
	}
	return true;
}

/**
 * Get colors as strings in format #rrggbb for the given mode and base color.
 */
bool
getColors(Mode mode, BaseColor base, uint intensity, float brightness,
	  std::vector<std::string> &colors)
{
	if (mode >= PALETTE_NO || base >= BASE_COLOR_NO
	    || intensity >= COLORS_PER_BASE) {
		return false;
	}

	std::vector<int> steps;
	uint idx = static_cast<uint>(base) * COLORS_PER_BASE + intensity;
	colors.push_back(getColor(idx, brightness));
	getSteps(mode, steps);
	std::vector<int>::iterator it(steps.begin());
	for (; it != steps.end(); ++it) {
		idx = (idx + *it * COLORS_PER_BASE) % COLORS_SIZE;
		colors.push_back(getColor(idx, brightness));
	}
	return true;
}

};
