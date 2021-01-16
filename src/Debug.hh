//
// Debug.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2012-2013 Andreas Schlick <ioerror@lavabit.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_DEBUG_HH_
#define _PEKWM_DEBUG_HH_

#include "config.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Debug {
public:
    static void doAction(const std::string &);

    static bool enable_cerr;
    static bool enable_logfile;

    static void setLogFile(const char *f) { _log.close(); _log.open(f); }
    static void addLog(const std::string s) {
        if (_msgs.size() >= _max_msgs) {
            _msgs.erase(_msgs.begin());
        }
        _msgs.push_back(s);
        if (enable_logfile) {
            _log << s << std::endl;
        }
        if (enable_cerr) {
            std::cerr << s << std::endl;
        }
    }
    static void addInfo(const std::string s) { addLog(_msg_info + s); }
    static void addWarn(const std::string s) { addLog(_msg_warn + s); }
    static void addErr(const std::string s)  { addLog(_msg_err + s); }

private:
    static std::ofstream _log;
    static std::vector<std::string> _msgs;
    static std::vector<std::string>::size_type _max_msgs;
    static const std::string _msg_info;
    static const std::string _msg_warn;
    static const std::string _msg_err;
    static const std::string _msg_bt;
};

class DebugDbgObj : public std::stringstream {
public:
    virtual ~DebugDbgObj() { Debug::addInfo(str()); }
};

class DebugInfoObj : public std::stringstream {
public:
    virtual ~DebugInfoObj() { Debug::addInfo(str()); }
};

class DebugWarnObj : public std::stringstream {
public:
    virtual ~DebugWarnObj() { Debug::addWarn(str()); }
};

class DebugErrObj : public std::stringstream {
public:
    virtual ~DebugErrObj() { Debug::addErr(str()); }
};

#define ___PEKDEBUG_START __PRETTY_FUNCTION__ << '@' << __LINE__ \
    << ":\n\t" << std::showbase << std::hex

#define DBG(M) \
    do { DebugDbgObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)

#define LOG(M) \
    do { DebugInfoObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define LOG_IF(C, M) \
    do { \
        if (C) { DebugInfoObj dobj; dobj << ___PEKDEBUG_START << M; } \
    } while (0)
#define WARN(M) \
    do { DebugWarnObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define ERR(M) \
    do { DebugErrObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define ERR_IF(C, M) \
    do { \
        if (C) { DebugErrObj dobj; dobj << ___PEKDEBUG_START << M; } \
    } while (0)

#endif // _PEKWM_DEBUG_HH_
