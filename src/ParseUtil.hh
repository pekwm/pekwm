//
// ParseUtil.hh for pekwm
// Copyright (C) 2004-2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PARSE_UTIL_HH_
#define _PARSE_UTIL_HH_

#include <map>
#include <string>
#include <cstring>

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

//! @brief Finds item in map, returns "" in map if not found
template<class T>
inline T
getValue(const std::string &text,
         typename std::map<ParseUtil::Entry, T> &val)
{
    typename std::map<ParseUtil::Entry, T>::iterator it = val.find(text);
    return (it != val.end()) ? it->second : val[""];
}

}

#endif // _PARSE_UTIL_HH_
