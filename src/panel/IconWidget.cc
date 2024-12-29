//
// IconWidget.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "Debug.hh"
#include "IconWidget.hh"
#include "TextFormatter.hh"

#include "../tk/ImageHandler.hh"

IconWidget::IconWidget(const PWinObj* parent,
		       const PanelTheme& theme,
		       const SizeReq& size_req,
		       VarData& var_data, WmState& wm_state,
		       const std::string& field,
		       const CfgParser::Entry *section)
	: PanelWidget(parent, theme, size_req),
	  _var_data(var_data),
	  _wm_state(wm_state),
	  _field(field),
	  _scale(false),
	  _icon(nullptr)
{
	parseIcon(section);

	pekwm::observerMapping()->addObserver(&_var_data, this, 100);
	load();
}

IconWidget::~IconWidget(void)
{
	if (_icon) {
		pekwm::imageHandler()->returnImage(_icon);
	}
	pekwm::observerMapping()->removeObserver(&_var_data, this);
}

void
IconWidget::notify(Observable *, Observation *observation)
{
	FieldObservation *fo = dynamic_cast<FieldObservation*>(observation);
	if (fo != nullptr && fo->getField() == _field) {
		_dirty = true;
		load();
	}
}

uint
IconWidget::getRequiredSize(void) const
{
	if (_scale || _icon == nullptr) {
		return _theme.getHeight();
	}
	return _icon->getWidth() + 2;
}

void
IconWidget::click(int, int)
{
	if (_pp_exec.size() == 0) {
		return;
	}

	TextFormatter tf(_var_data, _wm_state);
	std::string command = tf.format(_pp_exec);
	P_TRACE("IconWidget exec " << _exec << " command " << command);
	if (command.size() > 0) {
		std::vector<std::string> args =
			StringUtil::shell_split(command);
		Util::forkExec(args);
	}
}

void
IconWidget::render(Render& rend)
{
	PanelWidget::render(rend);
	if (_icon == nullptr) {
		// do nothing, no icon to render
	} else if (_scale) {
		renderScaled(rend);
	} else {
		renderFixed(rend);
	}
}

void
IconWidget::renderFixed(Render& rend)
{
	uint height, width;
	uint height_avail = _theme.getHeight() - 2;
	if (_icon->getHeight() > height_avail) {
		float aspect = (float) height_avail / _icon->getHeight();
		height = height_avail;
		width = _icon->getWidth() * aspect;
	} else {
		height = _icon->getHeight();
		width = _icon->getWidth();
	}
	_icon->draw(rend, getX() + 1, 1, width, height);
}

void
IconWidget::renderScaled(Render& rend)
{
	uint side = _theme.getHeight() - 2;
	_icon->draw(rend, getX() + 1, 1, side, side);
}

void
IconWidget::load(void)
{
	std::string value;
	if (! _field.empty()) {
		value = Charset::toSystem(_var_data.get(_field));
	}
	if (_transform.is_match_ok()) {
		_transform.ed_s(value);
	}
	std::string image = _name + "-" + value + _ext;
	if (value.empty() || ! loadImage(image)) {
		loadImage(_name + _ext);
	}
}

bool
IconWidget::loadImage(const std::string& icon_name)
{
	if (_icon_name == icon_name) {
		return true;
	}

	ImageHandler *ih = pekwm::imageHandler();
	if (_icon) {
		ih->returnImage(_icon);
	}
	_icon = ih->getImage(icon_name);
	if (_icon) {
		_icon_name = icon_name;
	} else {
		_icon_name = "";
	}
	return _icon != nullptr;
}

void
IconWidget::parseIcon(const CfgParser::Entry* section)
{
	std::string name, transform;
	CfgParserKeys keys;
	keys.add_string("ICON", name);
	keys.add_string("TRANSFORM", transform);
	keys.add_bool("SCALE", _scale);
	keys.add_string("EXEC", _exec);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	size_t pos = name.find_last_of(".");
	if (pos == std::string::npos) {
		_name = name;
	} else {
		_name = name.substr(0, pos);
		_ext = name.substr(pos, name.size() - pos);
	}
	if (transform.size() > 0) {
		_transform.parse_ed_s(transform);
	}

	TextFormatter tf(_var_data, _wm_state);
	_pp_exec = tf.preprocess(_exec);

	// inital load of image to get size request right
	load();
}
