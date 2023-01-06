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
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "RegexString.hh"
#include "VarData.hh"
#include "WmState.hh"

/**
 * Text widget with format string that is able to reference command
 * data, window manager state data and environment variables.
 *
 * Format $external, $:wm, $_env
 */
class TextWidget : public PanelWidget,
		   public Observer {
public:
	TextWidget(const PWinObj* parent,
		   const PanelTheme& theme, const SizeReq& size_req,
		   VarData& _var_data, WmState& _wm_state,
		   const std::string& format,
		   const CfgParser::Entry *section);
	virtual ~TextWidget(void);

	virtual void notify(Observable *, Observation *observation);
	virtual uint getRequiredSize(void) const;
	virtual void render(Render &rend);

private:
	void parseText(const CfgParser::Entry* section);

private:
	VarData& _var_data;
	WmState& _wm_state;
	std::string _pp_format;
	/** Regex transform of formatted output */
	RegexString _transform;

	bool _check_wm_state;
	std::vector<std::string> _fields;
};

#endif // _PEKWM_PANEL_TEXT_WIDGET_HH_
