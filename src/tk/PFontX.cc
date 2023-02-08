//
// PFontX.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PFontX.hh"
#include "Util.hh"

static const char* DEFAULT_SERIF = "courier";
static const char* DEFAULT_SANS = "helvetica";

static Util::StringTo<const char*> weight_map[] =
	{{"THIN", "light"},
	 {"EXTRALIGHT", "light"},
	 {"ULTRALIGHT", "light"},
	 {"LIGHT", "light"},
	 {"DEMILIGHT", "light"},
	 {"SEMILIGHT", "light"},
	 {"BOOK", "book"},
	 {"REGULAR", "regular"},
	 {"NORMAL", "medium"},
	 {"MEDIUM", "medium"},
	 {"DEMIBOLD", "demibold"},
	 {"SEMIBOLD", "semibold"},
	 {"BOLD", "bold"},
	 {"EXTRABOLD", "bold"},
	 {"BLACK", "bold"},
	 {"HEAVY", "bold"},
	 {nullptr, "medium"}};

static Util::StringTo<const char*> slant_map[] =
	{{"ITALIC", "i"},
	 {"OBLICQUE", "o"},
	 {"ROMAN", "r"},
	 {nullptr, "r"}};

static Util::StringTo<const char*> width_map[] =
	{{"ULTRACONDENSED", "condensed"},
	 {"EXTRACONDENSED", "condensed"},
	 {"CONDENSED", "condensed"},
	 {"SEMICONDENSED", "semicondensed"},
	 {"NORMAL", "normal"},
	 {"SEMIEXPANDED", "normal"},
	 {"EXPANDED", "normal"},
	 {"EXTRAEXPANDED", "normal"},
	 {"ULTRAEXPANDED", "normal"},
	 {nullptr, "*"}};



PFontX::PFontX(void)
	: PFont()
{
}

PFontX::~PFontX(void)
{
}

std::string
PFontX::toNativeDescr(const PFont::Descr &descr) const
{
	std::string family;
	uint size = 0;

	const std::vector<PFont::DescrFamily>& families = descr.getFamilies();
	std::vector<PFont::DescrFamily>::const_iterator fit(families.begin());
	for (; fit != families.end(); ++fit) {
		if (family.empty() && ! fit->getFamily().empty()) {
			family = fit->getFamily();
			Util::to_lower(family);
		}
		if (size == 0 && fit->getSize() != 0) {
			size = fit->getSize();
		}

		if (! family.empty() && size != 0) {
			break;
		}
	}

	if (family.empty() || family == "sans" || family == "sans serif") {
		family = DEFAULT_SANS;
	} else if (family == "serif") {
		family = DEFAULT_SERIF;
	}

	// override size with size property
	size = descr.getSize(size ? size : 12);

	const PFont::DescrProp* prop;

	prop = descr.getProperty("weight");
	const char *wght =
		Util::StringToGet(weight_map, prop ? prop->getValue() : "");
	prop = descr.getProperty("slant");
	const char *slant =
		Util::StringToGet(slant_map, prop ? prop->getValue() : "");
	prop = descr.getProperty("width");
	const char *sWdth =
		Util::StringToGet(width_map, prop ? prop->getValue() : "");

	std::ostringstream native;
	native << "-*" // fndry
	       << "-" << family
	       << "-" << wght
	       << "-" << slant
	       << "-" << sWdth
	       << "-*" // adstyl
	       << "-" << size
	       << "-*" // ptSz
	       << "-*" // resx
	       << "-*" // resy
	       << "-*" // spc
	       << "-*" // avgWdth
	       << "-*" // registry
	       << "-*"; // encdng
	return native.str();
}
