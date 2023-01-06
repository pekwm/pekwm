//
// IconWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_ICON_WIDGET_HH_
#define _PEKWM_PANEL_ICON_WIDGET_HH_

#include "pekwm_panel.hh"
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "PImage.hh"
#include "RegexString.hh"
#include "VarData.hh"
#include "WmState.hh"

/**
 * Display icon based on value from external command.
 */
class IconWidget : public PanelWidget,
		   public Observer {
public:
	IconWidget(const PWinObj* parent,
		   const PanelTheme& theme,
		   const SizeReq& size_req,
		   VarData &var_data,
		   WmState &wm_state,
		   const std::string& field,
		   const CfgParser::Entry *section);
	virtual ~IconWidget(void);

	virtual void notify(Observable *, Observation *observation);
	virtual uint getRequiredSize(void) const;
	virtual void click(int, int);
	virtual void render(Render& rend);

private:
	void renderFixed(Render& rend);
	void renderScaled(Render& rend);

	void load(void);
	bool loadImage(const std::string& icon_name);
	void parseIcon(const CfgParser::Entry* section);

private:
	VarData& _var_data;
	WmState& _wm_state;
	std::string _field;
	/** icon name, no file extension. */
	std::string _name;
	/** file extension. */
	std::string _ext;
	/** Regex transform of observed field */
	RegexString _transform;
	/** If true, scale icon square to fit panel height */
	bool _scale;
	/** If non empty, command to execute on click (release) */
	std::string _exec;
	/** Preprocessed version of exec string using TextFormatter. */
	std::string _pp_exec;

	/** current loaded icon, matching _icon_name. */
	PImage* _icon;
	std::string _icon_name;
};

#endif // _PEKWM_PANEL_ICON_WIDGET_HH_
