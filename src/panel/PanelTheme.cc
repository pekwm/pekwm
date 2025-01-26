//
// PanelTheme.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "PanelTheme.hh"

#include "../tk/FontHandler.hh"
#include "../tk/ImageHandler.hh"
#include "../tk/TextureHandler.hh"
#include "../tk/ThemeUtil.hh"

#define DEFAULT_BACKGROUND "SolidRaised #ffffff #eeeeee #cccccc"
#define DEFAULT_HEIGHT 24
#define DEFAULT_SEPARATOR "Solid #aaaaaa 1x24"

#define DEFAULT_COLOR "#000000"
#define DEFAULT_FONT "Sans-12"
#define DEFAULT_FONT_FOC DEFAULT_FONT ":weight=bold"
#define DEFAULT_FONT_ICO DEFAULT_FONT ":slant=talic"

#define DEFAULT_BAR_BORDER "black"
#define DEFAULT_BAR_FILL "grey50"

PanelTheme::PanelTheme(void)
	: _height(DEFAULT_HEIGHT),
	  _loaded(false),
	  _background(nullptr),
	  _background_opacity(255),
	  _sep(nullptr),
	  _handle(nullptr),
	  _bar_border(nullptr),
	  _bar_fill(nullptr)
{
	memset(_fonts, 0, sizeof(_fonts));
	memset(_colors, 0, sizeof(_colors));
}

PanelTheme::~PanelTheme(void)
{
	unload();
}

void
PanelTheme::load(const std::string &theme_dir, const std::string& theme_path)
{
	unload();

	CfgParser theme(CfgParserOpt(""));
	if (! ThemeUtil::load(theme, theme_dir, theme_path, true)) {
		check();
		return;
	}
	CfgParser::Entry *section = theme.getEntryRoot()->findSection("PANEL");
	if (section == nullptr) {
		check();
		return;
	}

	std::string background, separator, handle, bar_border, bar_fill;
	uint opacity;
	CfgParserKeys keys;
	keys.add_string("BACKGROUND", background, DEFAULT_BACKGROUND);
	keys.add_numeric<uint>("BACKGROUNDOPACITY", opacity, 100, 0, 100);
	keys.add_numeric<uint>("HEIGHT", _height, DEFAULT_HEIGHT);
	keys.add_string("SEPARATOR", separator, DEFAULT_SEPARATOR);
	keys.add_string("HANDLE", handle, "");
	keys.add_string("BARBORDER", bar_border, DEFAULT_BAR_BORDER);
	keys.add_string("BARFILL", bar_fill, DEFAULT_BAR_FILL);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	ImageHandler *ih = pekwm::imageHandler();
	ih->path_clear();
	ih->path_push_back(theme_dir + "/");

	TextureHandler *th = pekwm::textureHandler();
	_background = th->getTexture(background);
	if (opacity == 100) {
		_background_opacity = 255;
	} else {
		_background_opacity =
			static_cast<uchar>(255.0 * static_cast<float>(opacity)
					   / 100.0);
	}
	_sep = th->getTexture(separator);
	if (! handle.empty()) {
		_handle = th->getTexture(handle);
	}
	_bar_border = X11::getColor(bar_border);
	_bar_fill = X11::getColor(bar_fill);

	loadState(section->findSection("FOCUSED"), CLIENT_STATE_FOCUSED);
	loadState(section->findSection("UNFOCUSED"), CLIENT_STATE_UNFOCUSED);
	loadState(section->findSection("ICONIFIED"), CLIENT_STATE_ICONIFIED);

	check();
}

void
PanelTheme::unload(void)
{
	if (! _loaded) {
		return;
	}

	_height = DEFAULT_HEIGHT;
	X11::returnColor(_bar_border);
	_bar_border = nullptr;
	X11::returnColor(_bar_fill);
	_bar_fill = nullptr;
	TextureHandler *th = pekwm::textureHandler();
	if (_handle) {
		th->returnTexture(_handle);
		_handle = nullptr;
	}
	th->returnTexture(_sep);
	_sep = nullptr;
	th->returnTexture(_background);
	_background = nullptr;

	FontHandler *fh = pekwm::fontHandler();
	for (int i = 0; i < CLIENT_STATE_NO; i++) {
		fh->returnFont(_fonts[i]);
		_fonts[i] = nullptr;
		fh->returnColor(_colors[i]);
		_colors[i] = nullptr;
	}
	_loaded = false;
}

void
PanelTheme::loadState(CfgParser::Entry *section, ClientState state)
{
	if (section == nullptr) {
		return;
	}

	std::string font, color;
	CfgParserKeys keys;
	keys.add_string("FONT", font, DEFAULT_FONT);
	keys.add_string("COLOR", color, "#000000");
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	_fonts[state] = pekwm::fontHandler()->getFont(font);
	_colors[state] = pekwm::fontHandler()->getColor(color);
}

/**
 * Add configured path to image load path for looking up icons.
 */
void
PanelTheme::setIconPath(const std::string& config_path,
			const std::string& theme_path)
{
	ImageHandler *ih = pekwm::imageHandler();
	ih->path_push_back(DATADIR "/pekwm/icons/");
	ih->path_push_back(config_path);
	ih->path_push_back(theme_path);
}

/**
 * Get font and update color (same font will be shared)
 */
PFont*
PanelTheme::getFont(ClientState state) const
{
	PFont* font = _fonts[state];
	font->setColor(_colors[state]);
	return font;
}

void
PanelTheme::check(void)
{
	FontHandler *fh = pekwm::fontHandler();
	for (int i = 0; i < CLIENT_STATE_NO; i++) {
		if (_fonts[i] == nullptr) {
			P_TRACE("setting default font[" << i << "]");
			_fonts[i] = fh->getFont(DEFAULT_FONT "#Center");
		}
		if (_colors[i] == nullptr) {
			P_TRACE("setting default color[" << i << "]");
			_colors[i] = fh->getColor(DEFAULT_COLOR);
		}
	}

	TextureHandler *th = pekwm::textureHandler();
	if (_background == nullptr) {
		_background = th->getTexture(DEFAULT_BACKGROUND);
	}
	if (_sep == nullptr) {
		_sep = th->getTexture(DEFAULT_SEPARATOR);
	}

	_background->setOpacity(_background_opacity);
	if (_bar_border == nullptr) {
		_bar_border = X11::getColor(DEFAULT_BAR_BORDER);
	}
	if (_bar_fill == nullptr) {
		_bar_fill = X11::getColor(DEFAULT_BAR_FILL);
	}

	_loaded = true;
}
