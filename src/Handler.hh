//
// Handler.hh for pekwm
// Copyright (C) 2004-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_HANDLER_HH_
#define _PEKWM_HANDLER_HH_

#include "config.h"

#include "Types.hh"
#include "String.hh"

#include <string>
#include <cstring>

template<class T>
class HandlerEntry {
public:
	HandlerEntry(const std::string &name) : _name(name), _ref(0) { }
	virtual ~HandlerEntry(void) { }

	const std::string &getName(void) { return _name; }

	inline T getData(void) { return _data; }
	inline void setData(T data) { _data = data; }

	inline uint getRef(void) const { return _ref; }
	inline void incRef(void) { _ref++; }
	inline void decRef(void) { if (_ref > 0) { _ref--; }
	}

	inline bool operator==(const std::string &name) {
		return pekwm::ascii_ncase_equal(_name, name);
	}

private:
	std::string _name; // id
	uint _ref; // ref count

	T _data;
};

#endif // _PEKWM_HANDLER_HH_
