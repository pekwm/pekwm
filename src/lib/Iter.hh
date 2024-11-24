//
// Iter.hh for pekwm
// Copyright (C) 2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_ITER_HH_
#define _PEKWM_ITER_HH_

template<typename T, typename V>
class FilterIt
{
public:
	typedef bool(*filter_fun)(V v);

	FilterIt(T begin, T end, filter_fun filter_fun)
		: _it(begin),
		  _end(end),
		  _filter_fun(filter_fun)
	{
		skipFilter();
	}

	bool isValid() const { return _it != _end; }
	bool next()
	{
		if (isValid()) {
			++_it;
			return skipFilter();
		}
		return false;
	}
	T operator*() const { return _it; }

private:
	bool skipFilter()
	{
		while (isValid() && !filter(_it)) {
			++_it;
		}
		return isValid();
	}

	bool filter(T it)
	{
		return _filter_fun(*it);
	}

	T _it;
	T _end;
	filter_fun _filter_fun;
};

#endif // _PEKWM_ITER_HH_
