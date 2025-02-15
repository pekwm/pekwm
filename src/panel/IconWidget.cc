//
// IconWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "Debug.hh"
#include "IconWidget.hh"
#include "TextFormatter.hh"

#include "../tk/ImageHandler.hh"

IconWidget::IconWidget(const PanelWidgetData &data,
		       const PWinObj* parent,
		       const SizeReq& size_req,
		       const std::string& field,
		       const CfgParser::Entry *section)
	: PanelWidget(data, parent, size_req),
	  _field(field),
	  _scale(false),
	  _icon(nullptr),
	  _icon_scaled(nullptr)
{
	parseIcon(section);

	pekwm::observerMapping()->addObserver(&_var_data, this, 100);
	load();
}

IconWidget::~IconWidget(void)
{
	pekwm::observerMapping()->removeObserver(&_var_data, this);
	if (_icon) {
		pekwm::imageHandler()->returnImage(_icon);
	}
	delete _icon_scaled;
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
IconWidget::render(Render& rend)
{
	PanelWidget::render(rend);
	if (_icon == nullptr) {
		// do nothing, no icon to render
		P_TRACE("IconWidget render " << _name << _ext
			<< ", no icon loaded");
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
		width = static_cast<int>(_icon->getWidth() * aspect);
	} else {
		height = _icon->getHeight();
		width = _icon->getWidth();
	}
	P_TRACE("IconWidget render " << _icon_name << " " << width << "x"
		<< height << " (fixed)");
	scaleImage(width, height);
	_icon_scaled->draw(rend, getX() + 1, 1, width, height);
}

void
IconWidget::renderScaled(Render& rend)
{
	uint side = _theme.getHeight() - 2;
	P_TRACE("IconWidget render " << _icon_name << " " << side << "x"
		<< side << " (scaled)");
	scaleImage(side, side);
	_icon_scaled->draw(rend, getX() + 1, 1, side, side);
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
		P_TRACE("IconWidget loaded " << _icon_name << " "
			<< _icon->getWidth() << "x" << _icon->getHeight());
	} else {
		_icon_name = "";
	}

	// delete cache whenever a new image is loaded (or fails to do so)
	delete _icon_scaled;
	_icon_scaled = nullptr;

	return _icon != nullptr;
}

void
IconWidget::parseIcon(const CfgParser::Entry* section)
{
	std::string name, transform, exec;
	CfgParserKeys keys;
	keys.add_string("ICON", name);
	keys.add_string("TRANSFORM", transform);
	keys.add_bool("SCALE", _scale);
	keys.add_string("EXEC", exec);
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

	// legacy Exec configuration, now equal to Click = "1" { Exec = "..." }
	if (! exec.empty()) {
		TextFormatter tf(_var_data, _wm_state);
		std::string pp_exec = tf.preprocess(exec);
		PanelAction action(PANEL_ACTION_EXEC, exec, pp_exec);
		setButtonAction(1, action);
	}

	// inital load of image to get size request right
	load();
}

void
IconWidget::scaleImage(uint width, uint height)
{
	if (_icon_scaled == nullptr
	    || _icon_scaled->getWidth() != width
	    || _icon_scaled->getHeight() != height) {
		delete _icon_scaled;
		_icon_scaled = new PImage(_icon);
		_icon_scaled->scale(width, height);
	}
}
