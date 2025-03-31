//
// SysResources.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Calendar.hh"
#include "Debug.hh"
#include "SysResources.hh"
#include "X11.hh"

struct osc_xresource {
	int code1;
	int code2;
	const char *name;
};

static struct osc_xresource _osc_to_resource[] = {
	{ 4, 0, "XTerm*color0" },
	{ 4, 1, "XTerm*color1" },
	{ 4, 2, "XTerm*color2" },
	{ 4, 3, "XTerm*color3" },
	{ 4, 4, "XTerm*color4" },
	{ 4, 5, "XTerm*color5" },
	{ 4, 6, "XTerm*color6" },
	{ 4, 7, "XTerm*color7" },
	{ 4, 8, "XTerm*color8" },
	{ 4, 9, "XTerm*color9" },
	{ 4, 10, "XTerm*color10" },
	{ 4, 11, "XTerm*color11" },
	{ 4, 12, "XTerm*color12" },
	{ 4, 13, "XTerm*color13" },
	{ 4, 14, "XTerm*color14" },
	{ 4, 15, "XTerm*color15" },
	{ 10, -1, "XTerm*foreground" },
	{ 11, -1, "XTerm*background" },
	{ -1, -1, nullptr }
};

SysResources::SysResources(const SysConfig &cfg)
	: _cfg(cfg)
{
}

SysResources::~SysResources()
{
}

void
SysResources::update(const Daytime &daytime, TimeOfDay tod)
{
	const char *daylight;
	const char *theme_variant;
	switch (tod) {
	case TIME_OF_DAY_DAWN:
		daylight = "dawn";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_DUSK:
		daylight = "dusk";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_NIGHT:
		daylight = "night";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_DAY:
	default:
		daylight = "day";
		theme_variant = "light";
		break;
	};

	setXAtoms(theme_variant);
	setXResources(daytime, tod, daylight, theme_variant);
	notifyXTerms();
	X11::flush();
}

void
SysResources::setXAtoms(const char *theme_variant)
{
	X11::setString(X11::getRoot(), PEKWM_THEME_VARIANT, theme_variant);
}

void
SysResources::setXResources(const Daytime &daytime, TimeOfDay tod,
			    const char *daylight, const char *theme_variant)
{
	X11::loadXrmResources();
	X11::setXrmString("pekwm.daylight", daylight);
	X11::setXrmString("pekwm.theme.variant", theme_variant);
	X11::setXrmString("pekwm.location.latitude",
			  std::to_string(_cfg.getLatitude()));
	X11::setXrmString("pekwm.location.longitude",
			  std::to_string(_cfg.getLongitude()));
	X11::setXrmString("pekwm.location.sunrise",
			  Calendar(daytime.getSunRise()).toString());
	X11::setXrmString("pekwm.location.sunset",
			  Calendar(daytime.getSunSet()).toString());
	X11::setXrmString("pekwm.location.dawn",
			  Calendar(daytime.getDawn()).toString());
	X11::setXrmString("pekwm.location.night",
			  Calendar(daytime.getNight()).toString());
	X11::setXrmString("pekwm.location.country", _location_country);
	X11::setXrmString("pekwm.location.city", _location_city);

	const SysConfig::string_map &x_resources = _cfg.getXResources(tod);
	SysConfig::string_map::const_iterator it(x_resources.begin());
	for (; it != x_resources.end(); ++it) {
		X11::setXrmString(it->first, it->second);
	}
	P_TRACE("set " << x_resources.size()
		<< " X resources from configuration");

	X11::saveXrmResources();
}

void
SysResources::notifyXTerms()
{
	std::vector<Window> windows;
	readClientList(windows);
	P_TRACE("checking " << windows.size() << " windows for XTerm clients");

	std::vector<Window>::iterator it(windows.begin());
	for (; it != windows.end(); ++it) {
		X11::ClassHint class_hint;
		if (X11::getClassHint(*it, class_hint)
		    && strcmp("XTerm", class_hint.getClass()) == 0) {
			notifyXTerm(*it);
		}
	}
}

void
SysResources::notifyXTerm(Window win)
{
	P_TRACE("notify XTerm " << win << " using XTERM_CONTROL");

	XEvent ev = {};
	ev.type = ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.message_type = X11::getAtom(XTERM_CONTROL);
	ev.xclient.window = win;
	ev.xclient.format = 8;

	for (int i = 0; _osc_to_resource[i].name; i++) {
		std::string value;
		if (! X11::getXrmString(_osc_to_resource[i].name, value)) {
			continue;
		}
		if (_osc_to_resource[i].code2 == -1) {
			snprintf(ev.xclient.data.b, sizeof(ev.xclient.data.b),
				 "%d;%s", _osc_to_resource[i].code1,
				 value.c_str());
		} else {
			snprintf(ev.xclient.data.b, sizeof(ev.xclient.data.b),
				 "%d;%d;%s", _osc_to_resource[i].code1,
				 _osc_to_resource[i].code2, value.c_str());
		}
		X11::sendEvent(win, False, 0, &ev);
	}
}

bool
SysResources::readClientList(std::vector<Window> &windowsv)
{
	ulong actual;
	Window *windows;
	if (! X11::getProperty(X11::getRoot(),
			       X11::getAtom(PEKWM_CLIENT_LIST), XA_WINDOW, 0,
			       reinterpret_cast<uchar**>(&windows), &actual)) {
		return false;
	}

	windowsv.clear();
	for (ulong i = 0; i < actual; i++) {
		windowsv.push_back(windows[i]);
	}
	X11::free(windows);
	return true;
}
