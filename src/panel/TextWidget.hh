//
// TextWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_TEXT_WIDGET_HH_
#define _PEKWM_PANEL_TEXT_WIDGET_HH_

#include <string>

#include "Observable.hh"
#include "PanelWidget.hh"
#include "RegexString.hh"
#include "TextFormatter.hh"

/**
 * Text widget with format string that is able to reference command
 * data, window manager state data and environment variables.
 *
 * Format $external, $:wm, $_env
 */
class TextWidget : public PanelWidget {
public:
	TextWidget(const PanelWidgetData &data, const PWinObj* parent,
		   const WidgetConfig& cfg, const std::string& format);
	virtual ~TextWidget();

	virtual const char *getName() const { return "Text"; }

	virtual void notify(Observable *, Observation *observation);
	virtual uint getRequiredSize(void) const;
	virtual void render(Render &rend, PSurface* surface);

private:
	void parseText(const CfgParser::Entry* section);

	TextFormatObserver _tfo;
	/** Regex transform of formatted output */
	RegexString _transform;
};

#endif // _PEKWM_PANEL_TEXT_WIDGET_HH_
