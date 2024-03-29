//
// PanelWidget.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PanelWidget.hh"

PanelWidget::PanelWidget(const PWinObj* parent,
			 const PanelTheme& theme,
			 const SizeReq& size_req)
	: _parent(parent),
	  _theme(theme),
	  _dirty(true),
	  _x(0),
	  _rx(0),
	  _width(0),
	  _size_req(size_req)
{
}

PanelWidget::~PanelWidget(void)
{
}

void
PanelWidget::move(int x)
{
	_x = x;
	_rx = x + _width;
}

int
PanelWidget::renderText(Render &rend, PFont *font,
			int x, const std::string& text, uint max_width)
{
	int y = (_theme.getHeight() - font->getHeight()) / 2;
	Geometry gm(0, 0, _parent->getWidth(), _parent->getHeight());
	RenderSurface surface(rend, gm);
	return font->draw(&surface, x, y, text, 0, max_width);
}
