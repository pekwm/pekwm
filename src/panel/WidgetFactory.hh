//
// WidgetFactory.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_WIDGET_FACTORY_HH_
#define _PEKWM_PANEL_WIDGET_FACTORY_HH_

#include "pekwm_panel.hh"
#include "PanelConfig.hh"
#include "PanelTheme.hh"
#include "PanelWidget.hh"
#include "VarData.hh"
#include "WmState.hh"
#include "Os.hh"

/**
 * Widget construction.
 */
class WidgetFactory {
public:
	WidgetFactory(Os *os, PWinObj* parent, Observer* observer,
		      const PanelTheme& theme,
		      VarData& var_data, WmState& wm_state)
		: _data(os, observer, theme, var_data, wm_state),
		  _parent(parent)
	{
	}

	PanelWidget* construct(const WidgetConfig& cfg);

private:
	PanelWidget* mk(const WidgetConfig& cfg);
	void addClicks(const WidgetConfig& cfg, PanelWidget *widget);

	PanelWidgetData _data;
	PWinObj* _parent;
};

#endif // _PEKWM_PANEL_WIDGET_FACTORY_HH_
