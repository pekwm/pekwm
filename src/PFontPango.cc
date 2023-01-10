//
// PFontPango.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "PFontPango.hh"

static const char* FALLBACK_FONT_FAMILY = "Sans";
static const int FALLBACK_FONT_SIZE = 12 * PANGO_SCALE;

PFontPango::PFontPango()
	: PFont(),
	  _context(nullptr),
	  _font_map(nullptr),
	  _font(nullptr),
	  _font_description(nullptr)
{
}

PFontPango::~PFontPango()
{
	if (_font_map) {
		g_object_unref(_font_map);
	}
}

bool
PFontPango::load(const std::string& font_name)
{
	unload();

	_font_description =
		pango_font_description_from_string(font_name.c_str());

	// set fallback values to get a "sane" font description
	if (! pango_font_description_get_family(_font_description)) {
		USER_INFO("Pango font family fallback to "
			  << FALLBACK_FONT_FAMILY);
		pango_font_description_set_family(_font_description,
						  FALLBACK_FONT_FAMILY);
	}
	if (pango_font_description_get_size(_font_description) <= 0) {
		USER_INFO("Pango font size fallback to "
			  << FALLBACK_FONT_SIZE / PANGO_SCALE);
		pango_font_description_set_size(_font_description,
						FALLBACK_FONT_SIZE);
	}

	pango_context_set_font_description(_context, _font_description);
	_font = pango_context_load_font(_context, _font_description);

	if (_font) {
		PangoFontMetrics* metrics =
			pango_font_get_metrics(_font, nullptr);
		_ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
		_descent = pango_font_metrics_get_descent(metrics)
			/ PANGO_SCALE;
		_height = pango_font_metrics_get_height(metrics) / PANGO_SCALE;
		pango_font_metrics_unref(metrics);

		P_TRACE("loaded Pango font "
			<< pango_font_description_get_family(_font_description)
			<< pango_font_description_get_size(_font_description)
			   / PANGO_SCALE
			<< " height " << _height);
	} else {
		P_TRACE("failed to load Pango font " << font_name);
		pango_font_description_free(_font_description);
		_font_description = nullptr;
		return false;
	}

	return true;

}

void
PFontPango::unload()
{
	if (_font_description) {
		pango_font_description_free(_font_description);
		_font_description = nullptr;
	}
	if (_font) {
		g_object_unref(_font);
		_font = nullptr;
	}
}

/**
 * Get pango compatible number of characters from PFont characters.
 */
int
PFontPango::charsToLen(uint chars)
{
	return chars ? static_cast<int>(chars) : -1;
}
