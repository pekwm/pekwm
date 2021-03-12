//
// Debug.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2012-2013 Andreas Schlick <ioerror@lavabit.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "Compat.hh"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>

class Debug {
public:
    enum Level {
        LEVEL_ERR,
        LEVEL_WARN,
        LEVEL_INFO,
        LEVEL_DEBUG,
        LEVEL_TRACE
    };

    static Level getLevel(const std::string& str);
    static void doAction(const std::string& action);

    static bool enable_cerr;
    static bool enable_logfile;
    static Level level;

    static bool setLogFile(const char *f) {
        _log.close();
        _log.open(f);
        return _log.good();
    }
    static void addLog(const std::string s) {
        // keep max messages around in memory to aid in crash debugging
        if (_msgs.size() >= _max_msgs) {
            _msgs.erase(_msgs.begin());
        }
        _msgs.push_back(s);


        std::time_t t = std::time(nullptr);
        std::tm tm;
        localtime_r(&t, &tm);

        if (enable_logfile) {
            addTimestamp(_log, tm);
            _log << s << std::endl;
        }
        if (enable_cerr) {
            addTimestamp(std::cerr, tm);
            std::cerr << s << std::endl;
        }
    }

    static void addTimestamp(std::ostream &log, const std::tm& tm) {
        log << std::put_time(&tm, "%Y-%m-%d %H:%M:%S ");
    }

private:
    static std::ofstream _log;
    static std::vector<std::string> _msgs;
    static std::vector<std::string>::size_type _max_msgs;
};

class DebugUserObj : public std::stringstream {
public:
    DebugUserObj(void) : std::stringstream() { }
    virtual ~DebugUserObj(void) { Debug::addLog(str()); }
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
    fun << '@' << line << ":\n\t"

#define PEK_DEBUG_START \
    _PEK_DEBUG_START( __PRETTY_FUNCTION__, __LINE__)

#define USER_INFO(M) \
    do { DebugUserObj dobj; dobj << M; } while (0)

#define USER_WARN(M) \
    do { DebugUserObj dobj; dobj << "WARNING: " << M; } while (0)

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
