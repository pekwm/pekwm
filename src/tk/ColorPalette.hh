//
// ColorPalette.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"
#include "Compat.hh"
#include "Types.hh"
#include "X11.hh"

#include <vector>
#include <string>

namespace ColorPalette {

enum Mode {
	PALETTE_SINGLE,
	PALETTE_COMPLEMENTARY,
	PALETTE_TRIAD,
	PALETTE_ANALOGOUS,
	PALETTE_SPLIT,
	PALETTE_TETRAD,
	PALETTE_SQUARE,
	PALETTE_NO
};

enum BaseColor {
	BASE_COLOR_RED,
	BASE_COLOR_RED_PURPLE,
	BASE_COLOR_PURPLE,
	BASE_COLOR_BLUE_PURPLE,
	BASE_COLOR_BLUE,
	BASE_COLOR_BLUE_GREEN,
	BASE_COLOR_GREEN,
	BASE_COLOR_YELLOW_GREEN,
	BASE_COLOR_YELLOW,
	BASE_COLOR_YELLOW_ORANGE,
	BASE_COLOR_ORANGE,
	BASE_COLOR_RED_ORANGE,
	BASE_COLOR_NO
};

const uint MAX_INTENSITY = 4;

Mode modeFromString(const std::string &mode_str);
BaseColor baseColorFromString(const std::string &base_str);

bool getColors(Mode mode, BaseColor base, uint intensity,
	       std::vector<XColor*> &colors);
bool getColors(Mode mode, BaseColor base, uint intensity,
	       std::vector<std::string> &colors);

};
