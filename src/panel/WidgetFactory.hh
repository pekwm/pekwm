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
		: _os(os),
		  _parent(parent),
		  _observer(observer),
		  _theme(theme),
		  _var_data(var_data),
		  _wm_state(wm_state)
	{
	}

	PanelWidget* construct(const WidgetConfig& cfg);

private:
	Os* _os;
	PWinObj* _parent;
	Observer* _observer;
	const PanelTheme& _theme;
	VarData& _var_data;
	WmState& _wm_state;
};

#endif // _PEKWM_PANEL_WIDGET_FACTORY_HH_
