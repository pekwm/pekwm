//
// TextWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "TextFormatter.hh"
#include "TextWidget.hh"

TextWidget::TextWidget(const PanelWidgetData &data, const PWinObj* parent,
		       const WidgetConfig& cfg, const std::string& format)
	: PanelWidget(data, parent, cfg.getSizeReq(), cfg.getIf()),
	  _tfo(_var_data, _wm_state, this, format)
{
	parseText(cfg.getCfgSection());
}

TextWidget::~TextWidget()
{
}

void
TextWidget::notify(Observable *observable, Observation *observation)
{
	if (_dirty) {
		return;
	}

	FieldObservation *fo = dynamic_cast<FieldObservation*>(observation);
	_dirty = fo == nullptr || _tfo.match(observation);
	PanelWidget::notify(observable, observation);
}

uint
TextWidget::getRequiredSize(void) const
{
	if (_tfo.isFixed()) {
		// no variables that will be expanded after the widget has
		// been created, use width of _pp_format.
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		return font->getWidth(" " + _tfo.getPpFormat() + " ");
	}

	// variables will be expanded, no way to know how much space will
	// be required.
	return 0;
}

void
TextWidget::render(Render &rend, PSurface *surface)
{
	PanelWidget::render(rend, surface);

	PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
	std::string text(_tfo.format());
	if (_transform.is_match_ok()) {
		_transform.ed_s(text);
	}
	renderText(rend, surface, font, getX(), text, getWidth());
}

void
TextWidget::parseText(const CfgParser::Entry* section)
{
	std::string transform;
	CfgParserKeys keys;
	keys.add_string("TRANSFORM", transform);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	if (transform.size() > 0) {
		_transform.parse_ed_s(transform);
	}
}


