//
// DateTimeWidget.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_DATE_TIME_WIDGET_HH_
#define _PEKWM_PANEL_DATE_TIME_WIDGET_HH_

#include <string>

#include "PanelTheme.hh"
#include "PanelWidget.hh"

/**
 * Current Date/Time formatted using strftime.
 */
class DateTimeWidget : public PanelWidget {
public:
	DateTimeWidget(const PanelWidgetData &data, const PWinObj* parent,
		       const SizeReq& size_req,
		       const std::string &format)
		: PanelWidget(data, parent, size_req),
		  _format(format)
	{
		if (_format.empty()) {
			_format = "%Y-%m-%d %H:%M";
		}
	}

	virtual const char *getName() const { return "DateTime"; }

	virtual uint getRequiredSize(void) const
	{
		std::string stime;
		formatNow(stime);
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		return font->getWidth(" " + stime + " ");
	}

	virtual void render(Render &rend)
	{
		PanelWidget::render(rend);

		std::string stime;
		formatNow(stime);
		stime.insert(stime.begin(), ' ');
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		renderText(rend, font, getX(), stime, getWidth());

		// always treat date time as dirty, requires redraw up to
		// every second.
		_dirty = true;
	}

private:
	void formatNow(std::string &res) const
	{
		time_t now = time(NULL);
		struct tm tm;
		localtime_r(&now, &tm);

		char buf[64];
		strftime(buf, sizeof(buf), _format.c_str(), &tm);
		res = buf;
	}

private:
	std::string _format;
};


#endif // _PEKWM_PANEL_DATE_TIME_WIDGET_HH_
