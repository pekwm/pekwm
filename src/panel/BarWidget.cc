//
// BarWidget.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "BarWidget.hh"

BarWidget::BarWidget(const PWinObj* parent,
		     const PanelTheme& theme,
		     const SizeReq& size_req,
		     VarData& var_data,
		     const std::string& field,
		     const CfgParser::Entry *section)
	: PanelWidget(parent, theme, size_req),
	  _var_data(var_data),
	  _field(field)
{
	parseConfig(section);
	pekwm::observerMapping()->addObserver(&_var_data, this, 100);
}

BarWidget::~BarWidget(void)
{
	pekwm::observerMapping()->removeObserver(&_var_data, this);
}

void
BarWidget::render(Render &rend)
{
	PanelWidget::render(rend);

	int width = getWidth() - 3;
	int height = _theme.getHeight() - 4;
	rend.setColor(_theme.getBarBorder()->pixel);
	rend.rectangle(getX() + 1, 1, width, height);

	float fill_p = getPercent(_var_data.get(_field));
	int fill = static_cast<int>(fill_p * (height - 1));
	rend.setColor(getBarFill(fill_p));
	rend.fill(getX() + 2, 1 + height - fill, width - 1, fill);

	if (! _text.empty()) {
		PFont *font = _theme.getFont(CLIENT_STATE_UNFOCUSED);
		renderText(rend, font, getX() + 2, _text, width - 1);
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
