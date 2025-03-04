//
// PFontX.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PFontX.hh"
#include "PImage.hh"
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



PFontX::PFontX(float scale, PPixmapSurface &surface)
	: PFont(scale),
	  _gc_fg(None),
	  _gc_bg(None),
	  _pixel_fg(0),
	  _pixel_bg(0),
	  _surface(surface)
{
}

PFontX::~PFontX()
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

bool
PFontX::load(const PFont::Descr &descr)
{
	unload();

	// GC is created before loading the font to allow for setting the font
	// in the GC during the load
	XGCValues gv;
	uint mask = GCFunction;
	gv.function = GXcopy;
	_gc_fg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);
	_gc_bg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);

	std::string spec = descr.useStr() ? descr.str() : toNativeDescr(descr);
	if (! doLoadFont(spec)) {
		unload();
		return false;
	}
	return true;
}

/**
 * Unloads font resources
 */
void
PFontX::unload()
{
	if (_gc_fg != None) {
		XFreeGC(X11::getDpy(), _gc_fg);
		_gc_fg = None;
		_pixel_fg = 0;
	}
	if (_gc_bg != None) {
		XFreeGC(X11::getDpy(), _gc_bg);
		_gc_bg = None;
		_pixel_bg = 0;
	}
	doUnloadFont();
}

/**
 * Sets the color that should be used when drawing
 */
void
PFontX::setColor(PFont::Color *color)
{
	if (color->hasFg() && _pixel_fg != color->getFg()->pixel) {
		_pixel_fg = color->getFg()->pixel;
		XSetForeground(X11::getDpy(), _gc_fg, _pixel_fg);
	}
	if (color->hasBg() && _pixel_bg != color->getBg()->pixel) {
		_pixel_bg = color->getBg()->pixel;
		XSetForeground(X11::getDpy(), _gc_bg, _pixel_bg);
	}
}

void
PFontX::drawScaled(PSurface *dest, int x, int y, const std::string &text,
		   int size, GC gc, ulong color)
{
	// Get actual width, getWidth will returned the scaled value.
	uint width = doGetWidth(text, size);
	uint height = doGetHeight();
	Drawable sdraw = getShadowSurface(width, height);
	X11Render srender(sdraw);
	// Fill area with color treated as transparent to only retain the
	// actual text when blending onto the destination
	ulong trans_pixel = color == X11::getWhitePixel()
		? X11::getBlackPixel() : X11::getWhitePixel();
	srender.setColor(trans_pixel);
	srender.fill(0, 0, width, height);
	doDrawText(sdraw, 0, 0, text, size, gc);

	XImage *ximage = srender.getImage(0, 0, width, height);
	if (ximage) {
		PImage image(ximage, 255, &trans_pixel);
		X11::destroyImage(ximage);

		image.scale(_scale, PImage::SCALE_SQUARE);
		X11Render drender(dest->getDrawable());
		image.draw(drender, dest, x, y);
	}
}

/**
 * Get Drawable that can be used as an intermediate destination allowing
 * for access to the text pixels without any background.
 */
Drawable
PFontX::getShadowSurface(uint width, uint height)
{
	if (width > _surface.getWidth() || height > _surface.getHeight()) {
		width = std::max(width, _surface.getWidth());
		height = std::max(height, _surface.getHeight());
		_surface.resize(width, height);
	}
	return _surface.getDrawable();
}
