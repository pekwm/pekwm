//
// BarWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "BarWidget.hh"

BarWidget::BarWidget(const PanelWidgetData &data, const PWinObj* parent,
		     const WidgetConfig& cfg, const std::string& field,
		     const std::string& field_extra)
	: PanelWidget(data, parent, cfg.getSizeReq(), cfg.getIf()),
	  _field(field),
	  _field_extra(field_extra),
	  _checker_color(nullptr)
{
	parseConfig(cfg.getCfgSection());
	if (! _field_extra.empty()) {
		_checker_color = X11::getColor("#999999");
	}
	pekwm::observerMapping()->addObserver(&_var_data, this, 100);
}

BarWidget::~BarWidget()
{
	X11::returnColor(_checker_color);
	pekwm::observerMapping()->removeObserver(&_var_data, this);
}

void
BarWidget::render(Render &rend, PSurface *surface)
{
	PanelWidget::render(rend, surface);

	int width = getBarWidth();
	int height = getBarHeight();
	rend.setColor(_theme.getBarBorder()->pixel);
	rend.rectangle(getX() + 1, 1, width, height);
	renderFill(rend, surface, _field, false);
	if (! _field_extra.empty()) {
		renderFill(rend, surface, _field_extra, true);
	}

	if (! _text.empty()) {
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		renderText(rend, surface, font, getX() + 2, _text, width - 1);
	}
}

void
BarWidget::renderFill(Render &rend, PSurface *surface,
		      const std::string &field, bool checker)
{
	int width = getBarWidth();
	int height = getBarHeight();
	float fill_p = getPercent(_var_data.get(field));
	int fill = static_cast<int>(fill_p * (height - 1));

	int pixel = getBarFill(fill_p);
	if (checker) {
		rend.setFillStyle(RENDER_FILL_CHECKER);
		rend.setColor(_checker_color
			      ? _checker_color->pixel : X11::getWhitePixel());
		rend.fill(getX() + 2, 1 + height - fill, width - 1, fill);
		rend.setFillStyle(RENDER_FILL_SOLID);
	} else {
		rend.setColor(pixel);
		rend.fill(getX() + 2, 1 + height - fill, width - 1, fill);
	}
}

int
BarWidget::getBarFill(float percent) const
{
	std::vector<std::pair<float, XColor*> >::const_reverse_iterator it =
		_colors.rbegin();
	for (; it != _colors.rend(); ++it) {
		if (it->first <= percent) {
			break;
		}
	}

	if (it == _colors.rend()) {
		return _theme.getBarFill()->pixel;
	}
	return it->second->pixel;
}

float
BarWidget::getPercent(const std::string& str) const
{
	try {
		float percent = std::stof(str);
		if (percent < 0.0) {
			percent = 0.0;
		} else if (percent > 100.0) {
			percent = 100.0;
		}
		return percent / 100.0;
	} catch (std::invalid_argument&) {
		return 0.0;
	}
}

void
BarWidget::parseConfig(const CfgParser::Entry* section)
{
	CfgParserKeys keys;
	keys.add_string("TEXT", _text, "");
	section->parseKeyValues(keys.begin(), keys.end());
	parseColors(section);
}

void
BarWidget::parseColors(const CfgParser::Entry* section)
{
	CfgParser::Entry *colors = section->findSection("COLORS");
	if (colors == nullptr) {
		return;
	}

	CfgParser::Entry::entry_cit it = colors->begin();
	for (; it != colors->end(); ++it) {
		if (! pekwm::ascii_ncase_equal((*it)->getName(), "PERCENT")
		    || ! (*it)->getSection()) {
			continue;
		}

		float percent = getPercent((*it)->getValue());
		CfgParser::Entry *color =
			(*it)->getSection()->findEntry("COLOR");
		if (color) {
			addColor(percent, X11::getColor(color->getValue()));
		}
	}
}

void
BarWidget::addColor(float percent, XColor* color)
{
	std::vector<std::pair<float, XColor*> >::iterator it =
		_colors.begin();
	for (; it != _colors.end(); ++it) {
		if (percent < it->first) {
			break;
		}
	}

	_colors.insert(it, std::pair<float, XColor*>(percent, color));
}
