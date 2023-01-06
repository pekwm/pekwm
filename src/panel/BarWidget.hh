//
// BarWidget.hh for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_BAR_WIDGET_HH_
#define _PEKWM_PANEL_BAR_WIDGET_HH_

#include <string>
#include <vector>

#include "CfgParser.hh"
#include "Observable.hh"
#include "PanelWidget.hh"
#include "VarData.hh"

/**
 * Widget display a bar filled from 0-100% getting fill percentage
 * from external command data.
 */
class BarWidget : public PanelWidget,
		  public Observer {
public:
	BarWidget(const PWinObj* parent,
		  const PanelTheme& theme,
		  const SizeReq& size_req,
		  VarData& var_data,
		  const std::string& field,
		  const CfgParser::Entry *section);
	virtual ~BarWidget(void);

	virtual void notify(Observable*, Observation *observation)
	{
		FieldObservation *efo =
			dynamic_cast<FieldObservation*>(observation);
		if (efo != nullptr && efo->getField() == _field) {
			_dirty = true;
		}
	}

	virtual void render(Render &rend);

private:
	int getBarFill(float percent) const;
	float getPercent(const std::string& str) const;
	void parseColors(const CfgParser::Entry* section);
	void addColor(float percent, XColor* color);

private:
	VarData& _var_data;
	std::string _field;
	std::vector<std::pair<float, XColor*> > _colors;
};

#endif // _PEKWM_PANEL_BAR_WIDGET_HH_
