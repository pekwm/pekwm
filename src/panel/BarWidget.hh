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
// current value is chosen. Given the configuration below and a value of 80
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
//
// # Rendering with two values, extra-field on top of field is also possible.
// Bar = "field extra-field" {
// }
//
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
class BarWidget : public PanelWidget {
public:
	BarWidget(const PanelWidgetData &data, const PWinObj* parent,
		  const WidgetConfig& cfg, const std::string& field,
		  const std::string& field_extra);
	virtual ~BarWidget(void);

	const char *getName() const { return "Bar"; }

	virtual void notify(Observable* observable, Observation *observation)
	{
		FieldObservation *efo =
			dynamic_cast<FieldObservation*>(observation);
		if (efo != nullptr && efo->getField() == _field) {
			_dirty = true;
		}
		PanelWidget::notify(observable, observation);
	}

	virtual void render(Render &rend, PSurface *surface);

private:
	void renderFill(Render &rend, PSurface *surface,
			const std::string &field, bool checker);
	int getBarWidth() const { return getWidth() - 3; }
	int getBarHeight() const { return _theme.getHeight() - 4; }
	int getBarFill(float percent) const;
	float getPercent(const std::string& str) const;
	void parseConfig(const CfgParser::Entry* section);
	void parseColors(const CfgParser::Entry* section);
	void addColor(float percent, XColor* color);

	std::string _field;
	/** extra field value rendered on-top of field */
	std::string _field_extra;
	std::string _text;
	XColor *_checker_color;
	std::vector<std::pair<float, XColor*> > _colors;
};

#endif // _PEKWM_PANEL_BAR_WIDGET_HH_
