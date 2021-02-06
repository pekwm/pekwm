//
// Debug.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2012-2013 Andreas Schlick <ioerror@lavabit.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class Debug {
public:
    enum Level {
        LEVEL_ERR,
        LEVEL_WARN,
        LEVEL_INFO,
        LEVEL_DEBUG,
        LEVEL_TRACE

    };

    static void doAction(const std::string &);

    static bool enable_cerr;
    static bool enable_logfile;
    static Level level;

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

private:
    static std::ofstream _log;
    static std::vector<std::string> _msgs;
    static std::vector<std::string>::size_type _max_msgs;
};

class DebugTraceObj : public std::stringstream {
public:
    DebugTraceObj(void) : std::stringstream("TRACE: ") { }
    virtual ~DebugTraceObj(void) { Debug::addLog(str()); }
};

class DebugDbgObj : public std::stringstream {
public:
    DebugDbgObj(void) : std::stringstream("DEBUG: ") { }
    virtual ~DebugDbgObj(void) { Debug::addLog(str()); }
};

class DebugInfoObj : public std::stringstream {
public:
    DebugInfoObj(void) : std::stringstream("INFO: ") { }
    virtual ~DebugInfoObj(void) { Debug::addLog(str()); }
};

class DebugWarnObj : public std::stringstream {
public:
    DebugWarnObj(void) : std::stringstream("WARNING: ") { }
    virtual ~DebugWarnObj(void) { Debug::addLog(str()); }
};

class DebugErrObj : public std::stringstream {
public:
    DebugErrObj(void) : std::stringstream("ERROR: ") { }
    virtual ~DebugErrObj(void) { Debug::addLog(str()); }
};

#define _PEK_DEBUG_START(fun, line) \
    fun << '@' << line << ":\n\t" << std::showbase << std::hex

#define PEK_DEBUG_START \
    _PEK_DEBUG_START( __PRETTY_FUNCTION__, __LINE__)

#define USER_WARN(M) \
    do { DebugWarnObj dobj; dobj << M; } while (0)

#define TRACE(M) \
    if (Debug::level >= Debug::LEVEL_TRACE) { \
        DebugTraceObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define DBG(M) \
    if (Debug::level >= Debug::LEVEL_DEBUG) { \
        DebugDbgObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define LOG(M) \
    if (Debug::level >= Debug::LEVEL_INFO) { \
        DebugInfoObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define LOG_IF(C, M) \
    if ((C) && Debug::level >= Debug::LEVEL_INFO) { \
        DebugInfoObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define WARN(M) \
    if (Debug::level >= Debug::LEVEL_WARN) { \
        DebugWarnObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define ERR(M) \
    if (Debug::level >= Debug::LEVEL_ERR) { \
        DebugErrObj dobj; dobj << PEK_DEBUG_START << M; \
    }

#define ERR_IF(C, M) \
    if ((C) && Debug::level >= Debug::LEVEL_ERR) { \
        DebugErrObj dobj; dobj << PEK_DEBUG_START << M; \
    }
