//
// Geometry.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "Geometry.hh"

#include <iostream>

std::ostream&
operator<<(std::ostream& os, const Geometry& gm)
{
	os << "Geometry";
	os << " x:" << gm.x << " y: " << gm.y;
	os << " width: " << gm.width << " height: " << gm.height;
	return os;
}

Geometry::Geometry(void)
	: x(0),
	  y(0),
	  width(1),
	  height(1)
{
}

Geometry::Geometry(int _x, int _y, unsigned int _width, unsigned int _height)
	: x(_x),
	  y(_y),
	  width(_width),
	  height(_height)
{
}

Geometry::Geometry(const Geometry &gm)
	: x(gm.x),
	  y(gm.y),
	  width(gm.width),
	  height(gm.height)
{
}

Geometry::~Geometry(void)
{
}

/**
 * Construct a geometry from the given coordinates instead of position and
 * size.
 */
Geometry
Geometry::fromCordinates(int x, int y, int rx, int by)
{
	uint width = rx > x ? rx - x : 0;
	uint height = by > y ? by - y : 0;
	return Geometry(x, y, width, height);
}

/**
 * Update provided gm to be centered within this geometry.
 */
Geometry
Geometry::center(Geometry gm) const
{
	gm.x = x + (width / 2) - (gm.width / 2);
	gm.y = y + (height / 2) - (gm.height / 2);
	return gm;
}

Geometry&
Geometry::operator=(const Geometry& gm)
{
	x = gm.x;
	y = gm.y;
	width = gm.width;
	height = gm.height;
	return *this;
}

bool
Geometry::operator==(const Geometry& gm)
{
	return ((x == gm.x) && (y == gm.y) &&
		(width == gm.width) && (height == gm.height));
}

bool
Geometry::operator!=(const Geometry& gm)
{
	return (x != gm.x) || (y != gm.y) ||
		(width != gm.width) || (height != gm.height);
}

int
Geometry::diffMask(const Geometry &old_gm)
{
	int mask = 0;
	if (x != old_gm.x) {
		mask |= X_VALUE;
	}
	if (y != old_gm.y) {
		mask |= Y_VALUE;
	}
	if (width != old_gm.width) {
		mask |= WIDTH_VALUE;
	}
	if (height != old_gm.height) {
		mask |= HEIGHT_VALUE;
	}
	return mask;
}

static inline bool
_is_between(int val, int min, int max)
{
	return val > min && val < max;
}

static inline bool
_is_overlap(const Geometry &g1, const Geometry g2)
{
	return (_is_between(g2.x, g1.x, g1.rx())
		|| _is_between(g2.rx(), g1.x, g1.rx())
		|| (g2.x < g1.x && g2.rx() > g1.rx()))
		&& (_is_between(g2.y, g1.y, g1.by())
		    || _is_between(g2.by(), g1.y, g1.by())
		    || (g2.y < g1.y && g2.by() > g1.by()));
}

/**
 * Return area of Geometry;
 */
uint
Geometry::area() const
{
	return width * height;
}

/**
 * Return true if other geometry is covering this geometry.
 */
bool
Geometry::isCovered(const Geometry &other) const
{
	return other.x <= x && other.rx() >= rx()
		&& other.y <= y && other.by() >= by();
}

/**
 * Return true if other geometry is within the size and position of
 * this geometry.
 */
bool
Geometry::isOverlap(const Geometry &other) const
{
	return _is_overlap(*this, other) || _is_overlap(other, *this);
}

/**
 * Initate a GeometryOverlap calculation using base as the Geometry overlaps
 * will be added to.
 */
GeometryOverlap::GeometryOverlap(const Geometry &base)
	: _base(base),
	  _is_fully_covered(false)
{
	_rectangles.push_back(base);
}

GeometryOverlap::~GeometryOverlap()
{
}

/**
 * Get % of the base Geometry covered by overlaps.
 */
uint
GeometryOverlap::getOverlapPercent() const
{
	if (_is_fully_covered) {
		return 100;
	}

	double overlap_area = _base.area();
	std::vector<Geometry>::const_iterator it(_rectangles.begin());
	for (; it != _rectangles.end(); ++it) {
		overlap_area -= it->area();
	}

	return static_cast<uint>((overlap_area / _base.area()) * 100);
}

/**
 * Add overlap geometry.
 */
bool
GeometryOverlap::addOverlap(const Geometry &gm)
{
	if (! _base.isOverlap(gm)) {
		return false;
	}
	if (_base.isCovered(gm)) {
		_is_fully_covered = true;
		return true;
	}

	std::vector<Geometry>::iterator it(_rectangles.begin());
	while (it != _rectangles.end()) {
		it = addOverlap(gm, it);
	}

	return true;
}

GeometryOverlap::iterator
GeometryOverlap::addOverlap(const Geometry &gm, iterator it)
{
	Geometry &rectangle = *it;
	if (gm.x <= rectangle.x && gm.rx() >= rectangle.rx()) {
		it = addOverlapTrimHorizontal(gm, it);
	} else if (gm.y <= rectangle.y && gm.by() >= rectangle.by()) {
		it = addOverlapTrimVertical(gm, it);
	} else {
		it = addOverlapRect(gm, it);
	}
	return it;
}

GeometryOverlap::iterator
GeometryOverlap::addOverlapTrimHorizontal(const Geometry &gm, iterator it)
{
	// Full width, either shrink or shrink + create a new.
	//
	//    ------------- (r.y)
	//    |           |
	// ---------------- (gm.y)
	// |              |
	// ---------------- (gm.by())
	//    |           |
	//    ------------- (r.by)
	//
	Geometry &rectangle = *it;
	if (gm.y <= rectangle.y) {
		// shrink rectangle height from the top
		rectangle.height = rectangle.by() - gm.by();
		rectangle.y = gm.by();
	} else if (gm.by() >= rectangle.by()) {
		// shrink rectangle height from the bottom
		rectangle.height = gm.y - rectangle.y;
	} else {
		// shrink to middle and create a new part afterwards
		Geometry new_rectangle(rectangle.x, gm.by(),
				       rectangle.width,
				       rectangle.by() - gm.by());
		rectangle.height = gm.y - rectangle.y;
		it = _rectangles.insert(it + 1, new_rectangle);
	}
	return it + 1;
}

GeometryOverlap::iterator
GeometryOverlap::addOverlapTrimVertical(const Geometry &gm, iterator it)
{
	// Full height, either shrink or shrink + create a new
	//
	//     (gm.x) (gm.rx())
	//        -------
	//  (r.x) |     |  (r.rx())
	//    ----|-----|----
	//    |   |     |   |
	//    |   |     |   |
	//    |   |     |   |
	//    |   |     |   |
	//    ---------------
	//
	Geometry &rectangle = *it;
	if (gm.x <= rectangle.x) {
		// shrink rectangle width from left
		rectangle.width = rectangle.rx() - gm.rx();
		rectangle.x = gm.rx();
	} else if (gm.rx() >= rectangle.rx()) {
		// shrink rectangle width from rigth
		rectangle.width = gm.x - rectangle.x;
	} else {
		// shrink to middle and create a new part afterwards
		Geometry new_rectangle(gm.rx(), rectangle.y,
				       rectangle.rx() - gm.rx(),
				       rectangle.height);
		rectangle.width = gm.x - rectangle.x;
		it = _rectangles.insert(it + 1, new_rectangle);

	}
	return it + 1;
}

#define GFC(x, y, rx, by) Geometry::fromCordinates(x, y, rx, by)

GeometryOverlap::iterator
GeometryOverlap::addOverlapRect(const Geometry &gm, iterator it)
{
	// Replace current rectangle with up to 8 other rectangles.
	//
	// ---------------------
	// | 1 |     2     | 3 |
	// |-------------------|
	// |   |           |   |
	// | 4 |    gm     | 5 |
	// |   |           |   |
	// |-------------------|
	// | 6 |     7     | 8 |
	// ---------------------
	//
	Geometry r(*it);
	it = _rectangles.erase(it);

	int y;
	if (gm.y > r.y) {
		if (gm.x > r.x) {
			// 1
			Geometry nr = GFC(r.x, r.y, gm.x, gm.y);
			it = _rectangles.insert(it, nr) + 1;
		}
		// 2
		Geometry nr = GFC(std::max(r.x, gm.x), r.y,
				  std::min(r.rx(), gm.rx()), gm.y);
		it = _rectangles.insert(it, nr) + 1;
		if (gm.rx() < r.rx()) {
			// 3
			Geometry nr = GFC(gm.rx(), r.y, r.rx(), gm.y);
			it = _rectangles.insert(it, nr) + 1;
		}
		y = gm.y;
	} else {
		y = r.y;
	}

	if (gm.x > r.x) {
		// 4
		int rx = gm.x;
		int by = std::min(r.by(), gm.by());
		Geometry nr = GFC(r.x, y, rx, by);
		it = _rectangles.insert(it, nr) + 1;

	}
	if (gm.rx() < r.rx()) {
		// 5
		int rx = r.rx();
		int by = std::min(r.by(), gm.by());
		Geometry nr = GFC(gm.rx(), y, rx, by);
		it = _rectangles.insert(it, nr) + 1;

	}
	if (gm.by() < r.by()) {
		if (gm.x > r.x) {
			// 6
			Geometry nr = GFC(r.x, gm.by(), gm.x, r.by());
			it = _rectangles.insert(it, nr) + 1;
		}
		// 7
		Geometry nr = GFC(std::max(r.x, gm.x), gm.by(),
				  std::min(r.rx(), gm.rx()), r.by());
		it = _rectangles.insert(it, nr) + 1;
		if (gm.rx() < r.rx()) {
			// 8
			Geometry nr = GFC(gm.rx(), gm.by(), r.rx(), r.by());
			it = _rectangles.insert(it, nr) + 1;
		}
	}
	return it;
}
