//
// Handler.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifndef _HANDLER_HH_
#define _HANDLER_HH_

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
        return (strcasecmp(_name.c_str(), name.c_str()) == 0);
    }

private:
    std::string _name; // id
    uint _ref; // ref count

    T _data;
};

#endif // _HANDLER_HH_
