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
#include "TextFormatter.hh"
#include "SystrayWidget.hh"
#include "WidgetFactory.hh"

PanelWidget*
WidgetFactory::construct(const WidgetConfig& cfg)
{
	PanelWidget *widget = mk(cfg);
	if (widget && ! cfg.getClicks().empty()) {
		addClicks(cfg, widget);
	}
	return widget;
}

PanelWidget*
WidgetFactory::mk(const WidgetConfig& cfg)
{
	std::string name = cfg.getName();
	Util::to_upper(name);

	if (name == "BAR") {
		const std::string &field = cfg.getArg(0);
		if (field.empty()) {
			USER_WARN("missing required argument to Bar widget");
		} else {
			return new BarWidget(_data, _parent, cfg, field);
		}
	} else if (name == "CLIENTLIST") {
		const std::string &separator = cfg.getArg(0);
		return new ClientListWidget(_data, _parent, cfg, separator);
	} else if (name == "DATETIME") {
		const std::string &format = cfg.getArg(0);
		return new DateTimeWidget(_data, _parent, cfg, format);
	} else if (name == "ICON") {
		const std::string &field = cfg.getArg(0);
		return new IconWidget(_data, _parent, cfg, field);
	} else if (name == "SYSTRAY") {
		return new SystrayWidget(_data, _parent, cfg);

	} else if (name == "TEXT") {
		const std::string &format = cfg.getArg(0);
		if (format.empty()) {
			USER_WARN("missing required argument to Text widget");
		} else {
			return new TextWidget(_data, _parent, cfg, format);
		}
	} else {
		USER_WARN("unknown widget " << cfg.getName());
	}

	return nullptr;
}

void
WidgetFactory::addClicks(const WidgetConfig& cfg, PanelWidget* widget)
{
	TextFormatter tf(_data.var_data, _data.wm_state);

	const std::vector<WidgetConfigClick> &clicks = cfg.getClicks();
	std::vector<WidgetConfigClick>::const_iterator it(clicks.begin());
	for (; it != clicks.end(); ++it) {
		std::string pp_param = tf.preprocess(it->getParam());
		PanelAction action(it->getType(), it->getParam(), pp_param);
		widget->setButtonAction(it->getButton(), action);
	}
}
