//
// Util.hh for pekwm
// Copyright (C) 2002-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "Types.hh"

#include <algorithm>
#include <string>
#include <cstring>
#include <map>
#include <functional>
#include <sstream>
#include <vector>

extern "C" {
#include <string.h>
}

/**
 * String utilities, convenience functions for making life easier
 * when working with strings.
 */
namespace String {
    class Key {
    public:
        Key(const char *text) : _text(text) { }
        Key(const std::string &text) : _text(text.c_str()) { }
        ~Key(void) { }

        /** Get text version of string. */
        inline const char *get_text(void) const { return _text; }

        inline bool operator==(const std::string &rhs) const {
            return (strcasecmp(rhs.c_str(), _text) == 0);
        }
        inline bool operator!=(const std::string &rhs) const {
            return (strcasecmp(rhs.c_str(), _text) != 0);
        }
        inline bool operator<(const Key &rhs) const {
            return (strcasecmp(_text, rhs._text) < 0);
        }
        inline bool operator>(const Key &rhs) const {
            return (strcasecmp(_text, rhs._text) > 0);
        }

    private:
        const char *_text;
    };

    size_t safe_position(size_t pos, size_t fallback = 0, size_t add = 0);
    std::vector<std::string> shell_split(const std::string& str);
}

//! @brief Namespace Util used for various small file/string tasks.
namespace Util {
    template<class T>
    class StringMap : public std::map<String::Key, T> {
    public:
        using std::map<String::Key, T>::map;

        T get(const std::string& key) const {
            auto it = this->find(key);
            return it == this->end() ? this->at("") : it->second;
        }
    };

    std::string getEnv(const std::string& key);

    void forkExec(std::string command);
    pid_t forkExec(const std::vector<std::string>& args);
    std::string getHostname(void);

    bool isFile(const std::string &file);
    bool isExecutable(const std::string &file);
    time_t getMtime(const std::string &file);

    bool copyTextFile(const std::string &from, const std::string &to);

    std::string getUserName(void);

    std::string getFileExt(const std::string &file);
    std::string getDir(const std::string &file);
    void expandFileName(std::string &file);
    uint splitString(const std::string str,
                     std::vector<std::string> &vals,
                     const char *sep, uint max = 0,
                     bool include_empty = false, char escape = '\0');

    template<class T> std::string to_string(T t) {
        std::ostringstream oss;
        oss << t;
        return oss.str();
    }

    /**
     * Converts string to uppercase
     *
     * @param str Reference to the string to convert
     */
    inline void to_upper(std::string &str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       (int(*)(int)) std::toupper);
    }

    /**
     * Converts string to lowercase
     *
     * @param str Reference to the string to convert
     */
    inline void to_lower(std::string &str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       (int(*)(int)) std::tolower);
    }

    /**
     * Converts wide string to uppercase
     *
     * @param str Reference to the string to convert
     */
    inline void to_upper(std::wstring &str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       (int(*)(int)) std::towupper);
    }


    /**
     * Converts wide string to lowercase
     *
     * @param str Reference to the string to convert
     */
    inline void to_lower(std::wstring &str) {
        std::transform(str.begin(), str.end(), str.begin(),
                       (int(*)(int)) std::towlower);
    }

    std::string to_mb_str(const std::wstring &str);
    std::wstring to_wide_str(const std::string &str);

    void iconv_init(void);
    void iconv_deinit(void);

    /**
     * Return value within bounds of min and max value.
     */
    template<typename T>
    T between(T value, T min_val, T max_val) {
        if (value < min_val) {
            value = min_val;
        } else if (value > max_val) {
            value = max_val;
        }
        return value;
    }

    std::string to_utf8_str(const std::wstring &str);
    std::wstring from_utf8_str(const std::string &str);

    //! @brief Removes leading blanks( \n\t) from string.
    inline void trimLeadingBlanks(std::string &trim) {
        std::string::size_type first = trim.find_first_not_of(" \n\t");
        if ((first != std::string::npos) &&
            (first != (std::string::size_type) trim[0])) {
            trim = trim.substr(first, trim.size() - first);
        }
    }

    //! @brief Returns true if value represents true(1 or TRUE).
    inline bool isTrue(const std::string &value) {
        if (value.size() > 0) {
            if ((value[0] == '1') // check for 1 / 0
                || ! ::strncasecmp(value.c_str(), "TRUE", 4)) {
                return true;
            }
        }
        return false;
    }

    //! @brief for_each delete utility.
    template<class T> struct Free : public std::unary_function<T, void> {
        void operator ()(T t) { delete t; }
    };
}
