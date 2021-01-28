//
// ParseUtil.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include <map>
#include <string>
#include <cstring>

extern "C" {
#include <string.h>
}

namespace ParseUtil {

class Entry {
public:
    Entry(const char *text) : _text(text) { }
    // hacky, but we don't want to loose speed do we?
    Entry(const std::string &text) : _text(text.c_str()) { }
    ~Entry(void) { }

    /** Get text version of string. */
    inline const char *get_text(void) const { return _text; }

    // operators
    inline bool operator==(const std::string &rhs) const {
        return (strcasecmp(rhs.c_str(), _text) == 0);
    }
    inline bool operator!=(const std::string &rhs) const {
        return (strcasecmp(rhs.c_str(), _text) != 0);
    }
    // < > needed for map searching
    inline bool operator<(const ParseUtil::Entry &rhs) const {
        return (strcasecmp(_text, rhs._text) < 0);
    }
    inline bool operator>(const ParseUtil::Entry &rhs) const {
        return (strcasecmp(_text, rhs._text) > 0);
    }

private:
    const char *_text;
};

template<class T>
class Map : public std::map<ParseUtil::Entry, T> {
public:
    using std::map<ParseUtil::Entry, T>::map;

    T get(const std::string& key) const {
        auto it = this->find(key);
        return it == this->end() ? this->at("") : it->second;
    }
};

}
