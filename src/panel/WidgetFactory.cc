//
// WidgetFactory.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <string>

#include "BarWidget.hh"
#include "ClientListWidget.hh"
#include "DateTimeWidget.hh"
#include "Debug.hh"
#include "IconWidget.hh"
#include "TextWidget.hh"
#include "SystrayWidget.hh"
#include "WidgetFactory.hh"

PanelWidget*
WidgetFactory::construct(const WidgetConfig& cfg)
{
	std::string name = cfg.getName();
	Util::to_upper(name);

	if (name == "BAR") {
		const std::string &field = cfg.getArg(0);
		if (field.empty()) {
			USER_WARN("missing required argument to Bar widget");
		} else {
			return new BarWidget(_parent, _theme, cfg.getSizeReq(),
					     _var_data, field,
					     cfg.getCfgSection());
		}
	} else if (name == "CLIENTLIST") {
		const std::string &separator = cfg.getArg(0);
		return new ClientListWidget(_parent, _theme, cfg.getSizeReq(),
					    _wm_state, separator);
	} else if (name == "DATETIME") {
		const std::string &format = cfg.getArg(0);
		return new DateTimeWidget(_parent, _theme, cfg.getSizeReq(),
					  format);
	} else if (name == "ICON") {
		const std::string &field = cfg.getArg(0);
		return new IconWidget(_os, _parent, _theme, cfg.getSizeReq(),
				      _var_data, _wm_state,
				      field, cfg.getCfgSection());
	} else if (name == "SYSTRAY") {
		return new SystrayWidget(_parent, _observer, _theme,
					 cfg.getSizeReq(),
					 cfg.getCfgSection());

	} else if (name == "TEXT") {
		const std::string &format = cfg.getArg(0);
		if (format.empty()) {
			USER_WARN("missing required argument to Text widget");
		} else {
			return new TextWidget(_parent, _theme,
					      cfg.getSizeReq(), _var_data,
					      _wm_state, format,
					      cfg.getCfgSection());
		}
	} else {
		USER_WARN("unknown widget " << cfg.getName());
	}

	return nullptr;
}
