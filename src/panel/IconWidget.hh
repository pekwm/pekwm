//
// IconWidget.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_ICON_WIDGET_HH_
#define _PEKWM_PANEL_ICON_WIDGET_HH_

#include "pekwm_panel.hh"
#include "PanelWidget.hh"
#include "RegexString.hh"

#include "../tk/PImage.hh"

/**
 * Display icon based on value from external command.
 */
class IconWidget : public PanelWidget,
		   public Observer {
public:
	IconWidget(const PanelWidgetData &data, const PWinObj* parent,
		   const SizeReq& size_req,
		   const std::string& field,
		   const CfgParser::Entry *section);
	virtual ~IconWidget(void);

	virtual const char *getName() const { return "Icon"; }

	virtual void notify(Observable *, Observation *observation);
	virtual uint getRequiredSize(void) const;
	virtual void render(Render& rend);

private:
	void renderFixed(Render& rend);
	void renderScaled(Render& rend);

	void load(void);
	bool loadImage(const std::string& icon_name);
	void parseIcon(const CfgParser::Entry* section);
	void scaleImage(uint width, uint height);

private:
	std::string _field;
	/** icon name, no file extension. */
	std::string _name;
	/** file extension. */
	std::string _ext;
	/** Regex transform of observed field */
	RegexString _transform;
	/** If true, scale icon square to fit panel height */
	bool _scale;

	/** current loaded icon, matching _icon_name. */
	PImage* _icon;
	/** current loaded icon name. */
	std::string _icon_name;
	/** loaded icon, scaled to fit panel theme size. */
	PImage* _icon_scaled;
};

#endif // _PEKWM_PANEL_ICON_WIDGET_HH_
