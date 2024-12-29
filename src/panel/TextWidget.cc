//
// TextWidget.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "TextFormatter.hh"
#include "TextWidget.hh"

TextWidget::TextWidget(const PWinObj* parent,
		       const PanelTheme& theme, const SizeReq& size_req,
		       VarData& var_data, WmState& wm_state,
		       const std::string& format,
		       const CfgParser::Entry *section)
	: PanelWidget(parent, theme, size_req),
	  _var_data(var_data),
	  _wm_state(wm_state),
	  _check_wm_state(false)
{
	parseText(section);

	TextFormatter tf(_var_data, _wm_state);
	_pp_format = tf.preprocess(format);
	_check_wm_state = tf.referenceWmState();
	_fields = tf.getFields();

	if (! _fields.empty()) {
		pekwm::observerMapping()->addObserver(&_var_data, this, 100);
	}
	if (_check_wm_state) {
		pekwm::observerMapping()->addObserver(&_wm_state, this, 100);
	}
}

TextWidget::~TextWidget(void)
{
	if (_check_wm_state) {
		pekwm::observerMapping()->removeObserver(&_wm_state, this);
	}
	if (! _fields.empty()) {
		pekwm::observerMapping()->removeObserver(&_var_data, this);
	}
}

void
TextWidget::notify(Observable *, Observation *observation)
{
	if (_dirty) {
		return;
	}

	FieldObservation *fo = dynamic_cast<FieldObservation*>(observation);
	if (fo != nullptr) {
		std::vector<std::string>::iterator it = _fields.begin();
		for (; it != _fields.end(); ++it) {
			if (*it == fo->getField()) {
				_dirty = true;
				break;
			}
		}
	} else {
		_dirty = true;
	}
}

uint
TextWidget::getRequiredSize(void) const
{
	if (_fields.empty() && ! _check_wm_state) {
		// no variables that will be expanded after the widget has
		// been created, use width of _pp_format.
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		return font->getWidth(" " + _pp_format + " ");
	}

	// variables will be expanded, no way to know how much space will
	// be required.
	return 0;
}

void
TextWidget::render(Render &rend)
{
	PanelWidget::render(rend);

	TextFormatter tf(_var_data, _wm_state);
	PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
	std::string text = tf.format(_pp_format);
	if (_transform.is_match_ok()) {
		_transform.ed_s(text);
	}
	renderText(rend, font, getX(), text, getWidth());
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


