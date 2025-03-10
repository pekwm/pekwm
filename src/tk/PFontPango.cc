//
// PFontPango.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "PFontPango.hh"
#include "Util.hh"

#ifdef PEKWM_HAVE_PANGO

static const char* FALLBACK_FONT_FAMILY = "Sans";
static const int FALLBACK_FONT_SIZE = 12 * PANGO_SCALE;

static Util::StringTo<const char*> weight_map[] =
	{{"THIN", "Thin"},
	 {"EXTRALIGHT", "Extra-Light"},
	 {"ULTRALIGHT", "Ultra-Light"},
	 {"LIGHT", "Light"},
	 {"DEMILIGHT", "Demi-Light"},
	 {"SEMILIGHT", "Semi-Light"},
	 {"BOOK", "Book"},
	 {"REGULAR", "Regular"},
	 {"NORMAL", ""},
	 {"MEDIUM", "Medium"},
	 {"DEMIBOLD", "Demi-Bold"},
	 {"SEMIBOLD", "Semi-Bold"},
	 {"BOLD", "Bold"},
	 {"EXTRABOLD", "Extra-Bold"},
	 {"BLACK", "Black"},
	 {"HEAVY", "Heavy"},
	 {nullptr, ""}};

static Util::StringTo<const char*> width_map[] =
	{{"ULTRACONDENSED", "Ultra-Condensed"},
	 {"EXTRACONDENSED", "Extra-Condensed"},
	 {"CONDENSED", "Condensed"},
	 {"SEMICONDENSED", "Semi-Condensed"},
	 {"NORMAL", ""},
	 {"SEMIEXPANDED", "Semi-Expanded"},
	 {"EXPANDED", "Expanded"},
	 {"EXTRAEXPANDED", "Extra-Expanded"},
	 {"ULTRAEXPANDED", "Ultra-Expanded"},
	 {nullptr, ""}};

static void
ossAppend(std::ostringstream& oss, const std::string& str)
{
	if (str.empty()) {
		return;
	}
	if (oss.tellp() != 0) {
		oss << " ";
	}
	oss << str;
}

PFontPango::PFontPango(float scale)
	: PFont(scale),
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
PFontPango::load(const PFont::Descr& descr)
{
	unload();

	std::string spec = descr.useStr() ? descr.str() : toNativeDescr(descr);
	_font_description = pango_font_description_from_string(spec.c_str());

	// set fallback values to get a "sane" font description
	if (! pango_font_description_get_family(_font_description)) {
		USER_INFO("Pango font family fallback to "
			  << FALLBACK_FONT_FAMILY);
		pango_font_description_set_family(_font_description,
						  FALLBACK_FONT_FAMILY);
	}
	float scale = getScale();
	int size = pango_font_description_get_size(_font_description);
	if (size <= 0) {
		USER_INFO("Pango font size fallback to "
			  << FALLBACK_FONT_SIZE / PANGO_SCALE * scale);
		pango_font_description_set_size(_font_description,
						FALLBACK_FONT_SIZE * scale);
	} else if (scale != 1.0) {
		size = static_cast<int>(scale * size);
		pango_font_description_set_size(_font_description, size);
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
			<< " "
			<< pango_font_description_get_size(_font_description)
			   / PANGO_SCALE
			<< " height " << _height);
	} else {
		P_TRACE("failed to load Pango font " << descr.str());
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

bool
PFontPango::useAscentDescent(void) const
{
	return false;
}

std::string
PFontPango::toNativeDescr(const PFont::Descr &descr) const
{
	std::ostringstream native;

	uint size = 0;

	const std::vector<PFont::DescrFamily>& families = descr.getFamilies();
	std::vector<PFont::DescrFamily>::const_iterator fit(families.begin());
	for (; fit != families.end(); ++fit) {
		if (fit != families.begin()) {
			native << ',';
		}
		native << fit->getFamily();

		if (fit->getSize() > size) {
			size = fit->getSize();
		}
	}

	toNativeDescrAddStyle(descr, native);
	toNativeDescrAddWeight(descr, native);
	toNativeDescrAddStretch(descr, native);

	// override size with size property
	size = descr.getSize(size);
	if (size != 0) {
		ossAppend(native, std::to_string(size));
	}

	return native.str();
}

void
PFontPango::toNativeDescrAddStyle(const PFont::Descr& descr,
				  std::ostringstream& native) const
{
	const PFont::DescrProp* prop = descr.getProperty("slant");
	if (prop == nullptr) {
		return;
	}
	ossAppend(native, prop->getValue());
}

void
PFontPango::toNativeDescrAddWeight(const PFont::Descr& descr,
				   std::ostringstream& native) const
{
	const PFont::DescrProp* prop = descr.getProperty("weight");
	if (prop == nullptr) {
		return;
	}

	std::string weight = Util::StringToGet(weight_map, prop->getValue());
	ossAppend(native, weight);
}

void
PFontPango::toNativeDescrAddStretch(const PFont::Descr& descr,
				    std::ostringstream& native) const
{
	const PFont::DescrProp* width = descr.getProperty("width");
	if (width == nullptr) {
		return;
	}

	std::string stretch = Util::StringToGet(width_map, width->getValue());
	ossAppend(native, stretch);
}

/**
 * Get pango compatible number of characters from PFont characters.
 */
int
PFontPango::charsToLen(uint chars)
{
	return chars ? static_cast<int>(chars) : -1;
}

#endif // PEKWM_HAVE_PANGO
