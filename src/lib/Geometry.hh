//
// Geometry.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_GEOMETRY_HH_
#define _PEKWM_GEOMETRY_HH_

#include "config.h"
#include "Types.hh"

#include <vector>
#include <ostream>

/**
 * Bitmask values for parseGeometry result.
 */
enum GeometryMask {
	NO_VALUE = 0,
	X_VALUE = 1 << 0,
	Y_VALUE = 1 << 1,
	WIDTH_VALUE = 1 << 2,
	HEIGHT_VALUE = 1 << 3,
	ALL_VALUES = 1 << 4,
	X_NEGATIVE = 1 << 5,
	Y_NEGATIVE = 1 << 6,
	X_PERCENT = 1 << 7,
	Y_PERCENT = 1 << 8,
	WIDTH_PERCENT = 1 << 9,
	HEIGHT_PERCENT = 1 << 10
};

/**
 * Geometry (Rectangle).
 */
class Geometry {
public:
	Geometry(void);
	Geometry(int _x, int _y, unsigned int _width, unsigned int _height);
	Geometry(const Geometry &gm);
	~Geometry(void);

	static Geometry fromCordinates(int x, int y, int rx, int by);

	Geometry center(Geometry gm) const;

	int x;
	int y;
	uint width;
	uint height;

	inline int rx() const { return x + width; }
	inline int by() const { return y + height; }

	Geometry& operator=(const Geometry& gm);
	bool operator==(const Geometry& gm);
	bool operator != (const Geometry& gm);
	int diffMask(const Geometry &old_gm);

	uint area() const;
	bool isCovered(const Geometry &other) const;
	bool isOverlap(const Geometry &other) const;

	friend std::ostream& operator<<(std::ostream& os, const Geometry& gm);
};

/**
 * Class for calculating overlap between a base Geometry and N amount of
 * Geometries added on top.
 */
class GeometryOverlap {
public:
	typedef std::vector<Geometry>::iterator iterator;

	GeometryOverlap(const Geometry &base);
	~GeometryOverlap();

	uint getOverlapPercent() const;

	bool addOverlap(const Geometry &gm);

private:
	iterator addOverlap(const Geometry &gm, iterator it);
	iterator addOverlapTrimHorizontal(const Geometry &gm, iterator it);
	iterator addOverlapTrimVertical(const Geometry &gm, iterator it);
	iterator addOverlapRect(const Geometry &gm, iterator it);

	/** Base geometry that overlaps are added to. */
	Geometry _base;
	/** If true, all of base is already overlap. */
	bool _is_fully_covered;
	/** Geometries. */
	std::vector<Geometry> _rectangles;

};

#endif // _PEKWM_GEOMETRY_HH_
