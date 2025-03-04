//
// BarWidget.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

// DOC
//
// Renders a filled rectangle, with an optional text string in it, that is
// changing color depending on the fill level.
//
// The field should be a value between 0 and 100.
//
// Color is selected based on the value, first color with a value below the
// current value is choosen. Given the configuration below and a value of 80
// #cccc00 will be selected.
//
// ```
// Bar = "field" {
//   Size = "Pixels 24"
//   Text = "F"
//   Colors {
//    Percent = "0" {
//      Color = "#00cc00"
//    }
//    Percent = "75" {
//      Color = "#cccc00"
//    }
//    Percent = "90" {
//      Color = "#cc0000"
//    }
//   }
// }
// ```
// ENDDOC


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
	BarWidget(const PanelWidgetData &data, const PWinObj* parent,
		  const SizeReq& size_req,
		  const std::string& field,
		  const CfgParser::Entry *section);
	virtual ~BarWidget(void);

	const char *getName() const { return "Bar"; }

	virtual void notify(Observable*, Observation *observation)
	{
		FieldObservation *efo =
			dynamic_cast<FieldObservation*>(observation);
		if (efo != nullptr && efo->getField() == _field) {
			_dirty = true;
		}
	}

	virtual void render(Render &rend, PSurface *surface);

private:
	int getBarFill(float percent) const;
	float getPercent(const std::string& str) const;
	void parseConfig(const CfgParser::Entry* section);
	void parseColors(const CfgParser::Entry* section);
	void addColor(float percent, XColor* color);

	std::string _field;
	std::string _text;
	std::vector<std::pair<float, XColor*> > _colors;
};

#endif // _PEKWM_PANEL_BAR_WIDGET_HH_
