//
// Util.hh for pekwm
// Copyright © 2002-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _UTIL_HH_
#define _UTIL_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Types.hh"

#include <string>
#include <cstring>
#include <vector>
#include <list>
#include <functional>
#include <sstream>

//! @brief Namespace Util used for various small file/string tasks.
namespace Util {
    void forkExec(std::string command);
    bool isFile(const std::string &file);
    bool isExecutable(const std::string &file);
    bool isFileChanged(const std::string &file, time_t &mtime);
    bool requireReload(std::string &old_file, const std::string &new_file,  time_t &mtime);

    bool copyTextFile(const std::string &from, const std::string &to);

    std::string getFileExt(const std::string &file);
    std::string getDir(const std::string &file);
    void expandFileName(std::string &file);
    uint splitString(const std::string &str, std::vector<std::string> &vals, const char *sep, uint max = 0);

    template<class T> std::string to_string(T t) {
        std::ostringstream oss;
        oss << t;
        return oss.str();
    }

    void to_upper(std::string &str);
    void to_lower(std::string &str);

  std::string to_mb_str(const std::wstring &str);
  std::wstring to_wide_str(const std::string &str);

  void iconv_init(void);
  void iconv_deinit(void);

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
                || ! strncasecmp(value.c_str(), "TRUE", 4)) {
                return true;
            }
        }
        return false;
    }

    //! @brief for_each delete utility.
    template<class T> struct Free : public std::unary_function<T, void> {
        void operator ()(T t) { delete t; }

    };

  /**
   * File backed string list used to persist, amongst other things
   * command history.
   */
  class file_backed_list : public std::list<std::wstring>
  {
  public:
    /** Return path list is backed up by. */
    const std::string &get_path (void) const { return _path; }
    /** Set path list is backed up by. */
    void set_path (const std::string &path) { _path = path; }

    void push_back_unique(const std::wstring &entry);

    unsigned int load (const std::string &path);
    bool save (const std::string &path);

  private:
    std::string _path; /**< Path to file backed version. */
  };

}

#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif // HAVE_SETENV

#ifndef HAVE_TIMERSUB
#define timersub(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif // HAVE_TIMERSUB

#endif // _UTIL_HH_
