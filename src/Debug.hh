//
// Debug.hh for pekwm
// Copyright © 2012 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_DEBUG_HH
#define _PEKWM_DEBUG_HH

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <iostream>

#define ___PEKDEBUG_START __PRETTY_FUNCTION__ << " [" << __LINE__ << "]: "

#ifdef DEBUG
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
    static const std::string _msg_info, _msg_warn, _msg_err;
};

class DebugInfoObj : public std::stringstream {
public:
    DebugInfoObj() { *this << std::showbase << std::hex; }
    virtual ~DebugInfoObj() { Debug::addInfo(str()); }
};
class DebugWarnObj : public std::stringstream {
public:
    DebugWarnObj() { *this << std::showbase << std::hex; }
    virtual ~DebugWarnObj() { Debug::addWarn(str()); }
};
class DebugErrObj : public std::stringstream {
public:
    DebugErrObj() { *this << std::showbase << std::hex; }
    virtual ~DebugErrObj() { Debug::addErr(str()); }
};

class DebugFuncCall {
public:
    DebugFuncCall(const char *func) : _fname(func) { logName(); ++_depth; }
    ~DebugFuncCall() { --_depth; }

private:
    void logName(void) {
        std::string msg("FCall: ");
        for (unsigned int i=0; i < _depth; ++i)
            msg += "  ";
        msg += "+-" + _fname;
        Debug::addLog(msg);
    }
    std::string _fname;
    static unsigned int _depth;
};
#define LOG_CALL() DebugFuncCall ___PEKDEBUG_VAR##__FUNCTION__(__PRETTY_FUNCTION__)

#define LOG(M) do { DebugInfoObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define LOG_IF(C, M) do { if (C) { DebugInfoObj dobj; dobj << ___PEKDEBUG_START << M; } } while (0)
#define LOG_IFE(C, M1, M2) do { DebugInfoObj dobj; dobj << ___PEKDEBUG_START << ((C)?M1:M2); } while (0)
#define WARN(M) do { DebugWarnObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define ERR(M) do { DebugErrObj dobj; dobj << ___PEKDEBUG_START << M; } while (0)
#define ERR_IF(C, M) do { if (C) { DebugErrObj dobj; dobj << ___PEKDEBUG_START << M; } } while (0)

#else
#define LOG_CALL() do { (void)0; } while (0)
#define LOG(M) do { (void)0; } while (0)
#define LOG_IF(C,M) do { (void)0; } while (0)
#define LOG_IFE(C, M1, M2) do { (void)0; } while (0)
#define WARN(M) do { std::cerr << " *WARNING* " << ___PEKDEBUG_START << M << std::endl; } while (0)
#define ERR(M) do { std::cerr << " *ERROR* " << ___PEKDEBUG_START << M << std::endl; } while (0)
#define ERR_IF(C, M) do { if (C) { std::cerr << " *ERROR* " << ___PEKDEBUG_START << M << std::endl; } } while (0)
#endif // DEBUG

#endif // _PEKWM_DEBUG_HH
