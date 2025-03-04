//
// TextureLinesAngle.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TEXTURE_LINES_ANGLE_HH_
#define _TEXTURE_LINES_ANGLE_HH_

#include <cmath>

#include "../tk/Color.hh"
#include "../tk/PTexture.hh"

/**
 * Render filled lines, much like LinesHorz and LinesVert but lines are
 * angled at X degrees.
 *
 * 0-90 degrees as below, 90-180 same but directed from left to right.
 *
 * -----------------
 * | /  /  /  /  / |
 * |/  /  /  /  /  |
 * |  /  /  /  /  /|
 * | /  /  /  /  / |
 * -----------------
 *
 */
class TextureLinesAngle : public PTexture {
public:
	TextureLinesAngle(float line_size, bool size_percent, double angle,
			  std::vector<std::string>::const_iterator cbegin,
			  std::vector<std::string>::const_iterator cend)
		: _line_size(line_size),
		  _size_percent(size_percent),
		  _angle(angle)
	{
		setColors(cbegin, cend);
	}

	virtual ~TextureLinesAngle()
	{
	}

	virtual void doRender(Render &rend,
			      int x, int y, size_t width, size_t height)
	{
		int step;
		if (_size_percent) {
			step = width * _line_size;
		} else {
			step = _line_size;
		}

		rend.setClip(x, y, width, height);

		double angle = _angle > 90.0 ? _angle - 90.0 : _angle;
		double radians = angle * (M_PI / 180.0);
		ushort side = static_cast<double>(height) / tan(radians);

		if (_angle > 90.0) {
			doRenderLtoR(rend, x, y, width, height, step, side);
		} else {
			doRenderRtoL(rend, x, y, width, height, step, side);
		}
	}
	virtual bool getPixel(ulong &pixel) const
	{
		return false;
	}

private:
	void doRenderRtoL(Render &rend,
			 int x, int y, size_t width, size_t height,
			 int step, ushort side)
	{
		int lx = x - (step / 2);
		int rx = x + width;

		int bx;
		std::vector<XColor*>::iterator it(_colors.begin());
		do {
			rend.setColor((*it)->pixel);
			bx = renderLineRtoL(rend, lx, y, step, side, height);
			lx += step;
			if (++it == _colors.end()) {
				it = _colors.begin();
			}
		} while (bx < rx);
		rend.clearClip();
	}

	int renderLineRtoL(Render &rend, short x, short y, ushort size,
			   ushort side, ushort height)
	{
		point_vector points;
		points.add(x, y);
		points.add(x + size, y);
		points.add(x + size - side, y + height);
		points.add(x - side, y + height);
		points.add(x, y);
		rend.poly(points, true /* fill */);
		return x - side;
	}

	void doRenderLtoR(Render &rend,
			  int x, int y, size_t width, size_t height,
			  int step, ushort side)
	{
		int lx = x + width + (step / 2);

		int bx;
		std::vector<XColor*>::iterator it(_colors.begin());
		do {
			rend.setColor((*it)->pixel);
			bx = renderLineLtoR(rend, lx, y, step, side, height);
			lx -= step;
			if (++it == _colors.end()) {
				it = _colors.begin();
			}
		} while (bx > x);
		rend.clearClip();
	}

	int renderLineLtoR(Render &rend, short x, short y, ushort size,
			    ushort side, ushort height)
	{
		point_vector points;
		points.add(x, y);
		points.add(x + size, y);
		points.add(x + size + side, y + height);
		points.add(x + side, y + height);
		points.add(x, y);
		rend.poly(points, true /* fill */);
		return x + side;
	}

	void setColors(std::vector<std::string>::const_iterator cbegin,
		       std::vector<std::string>::const_iterator cend)
	{
		unsetColors();

		std::vector<std::string>::const_iterator it(cbegin);
		for (; it != cend; ++it) {
			_colors.push_back(pekwm::getColor(*it));
		}
		_ok = ! _colors.empty();
	}

	void unsetColors()
	{
		std::vector<XColor*>::iterator it(_colors.begin());
		for (; it != _colors.end(); ++it) {
			X11::returnColor(*it);
		}
		_colors.clear();
	}

	/** Line width/height, given in percent (0-100) if _size_percent is
	 * true */
	float _line_size;
	/** If true, size given in percent instead of pixels. */
	bool _size_percent;
	/** Line angle */
	double _angle;
	/** Line colors. */
	std::vector<XColor*> _colors;
};

PTexture*
parseLinesAngle(float scale, const std::string &,
		const std::vector<std::string> &tok)
{
	if (tok.size() < 3) {
		return nullptr;
	}

	float line_size;
	bool size_percent;
	try {
		if (tok[0][tok[0].size()-1] == '%') {
			size_percent = true;
			std::string size = tok[0].substr(0, tok[0].size() - 1);
			line_size = std::stof(size) / 100;
		} else {
			size_percent = false;
			line_size = std::stoi(tok[0]);
		}
	} catch (const std::invalid_argument&) {
		return nullptr;
	}

	double angle;
	try {
		angle = std::stod(tok[1]);
		if (angle <= 0.0 || angle == 90.0 || angle >= 180.0) {
			throw std::invalid_argument(
				"angle must be > 0.0 and < 180.0, "
				"excluding 90.0");
		}
	} catch (const std::invalid_argument&) {
		return nullptr;
	}

	std::vector<std::string>::const_iterator cend(tok.end());
	uint width = 0;
	uint height = 0;
	if (X11::parseSize(tok.back(), width, height)) {
		--cend;
	}

	PTexture *tex = new TextureLinesAngle(line_size, size_percent, angle,
					      tok.begin() + 2, cend);
	tex->setWidth(width);
	tex->setHeight(height);
	return tex;
}

#endif // _TEXTURE_LINES_ANGLE_HH_
