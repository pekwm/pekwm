//
// Debug.cc for pekwm
// Copyright © 2012 Andreas Schlick <ioerror{@}lavabit{.}com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "Debug.hh"
#include "Util.hh"

#include <cstdlib>
#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
#include <execinfo.h>
#include <cxxabi.h>
#endif

/**
 * Debug Commands:
 *
 * enable [logfile|cerr] - enable logging to [logfile|std::cerr]
 * disable [logfile|cerr] - disable logging to [logfile|std::cerr]
 * toggle [logfile|cerr] - toggle logging to [logfile|std::cerr]
 * logfile <filename> - open filename as the logfile
 *                      (this always closes the old logfile)
 * dump - write all stored log messages to the logfile
 * maxmsgs - log the current maximum number of stored messages
 * maxmsgs <nr> - sets the maximum of stored messages (in RAM) to nr
 *
 */
void Debug::doAction(const std::string &cmd) {
    vector<std::string> args;

    uint nr = Util::splitString(cmd, args, " \t");
    if (nr)
        Util::to_lower(args[0]);

    if (nr == 1) {
        if (args[0] == "dump") {
            if (! _log.is_open())
                return;
            _log << "--- DUMPING LOG ---" << std::endl;
            for (unsigned i=0; i < _msgs.size(); ++i) {
                _log << i << ".) " << _msgs[i] << std::endl;
            }
        } else if (args[0] == "maxmsgs") {
            _log << "Currently are " << _max_msgs << " log entries stored." << std::endl;
        }
        return;
    }

    if (nr == 2) {
        if (args[0] == "enable") {
            if (args[1] == "logfile") {
                enable_logfile = true;
            } else if (args[1] == "cerr") {
                enable_cerr = true;
            }
        } else if (args[0] == "disable") {
            if (args[1] == "logfile") {
                enable_logfile = false;
            } else if (args[1] == "cerr") {
                enable_cerr = false;
            }
        } else if (args[0] == "toggle") {
            if (args[1] == "logfile") {
                enable_logfile = ! enable_logfile;
            } else if (args[1] == "cerr") {
                enable_cerr = ! enable_cerr;
            }
        } else if (args[0] == "logfile") {
            setLogFile(args[1].c_str());
            if (! _log.is_open()) {
                enable_logfile = false;
            }
        } else if (args[0] == "maxmsgs") {
            int nr = std::atoi(args[1].c_str());
            if (nr>=0) {
                _max_msgs = nr;
                if (_msgs.size() > _max_msgs) {
                    _msgs.erase(_msgs.begin(), _msgs.begin() + _msgs.size() - _max_msgs);
                }
            } else {
                WARN("Debug command \"maxmsgs\" called with wrong parameter.");
            }
        }
        return;
    }
}

#if defined(__GLIBCXX__) || defined(__GLIBCPP__)
static
const char *demangle_cpp(const char *str, char **dest, size_t *len)
{
    int status=1;
    char *begin = const_cast<char*>(strchr(str, '(')), *end;
    if (begin && *(++begin) && *begin != '+') {
        end = strchr(begin, '+');
        if (end) {
            char *buf = new char[end-begin+1];
            memcpy(buf, begin, end-begin);
            buf[end-begin] = 0;
            *dest = abi::__cxa_demangle(buf, *dest, len, &status);
            delete[] buf;
        }
    }
    return status?str:*dest;
}

void Debug::logBacktrace(DebugBTObj &dobj) {
    void *btbuffer[100];
    char *name=0, **str=0;
    size_t len=0;
    int size = backtrace(btbuffer, 100);

    if (! size) {
        dobj << "Generating backtrace failed!";
        return;
    }

    str = backtrace_symbols(btbuffer, size);

    if (! str) {
        dobj << "Translating backtrace failed!";
        return;
    }

    // The first entry is always Debug::logBacktrace(),
    // so we begin with i=1.
    for (int i=1; i<size; ++i) {
        dobj << "\t" << demangle_cpp(str[i], &name, &len) << '\n';
    }

    free(name);
    free(str);
}
#else
void Debug::logBacktrace(DebugInfoObj &dobj) {
    dobj << "Backtrace works only with glibc.\n";
}
#endif

bool Debug::enable_cerr = true;
bool Debug::enable_logfile = false;
std::ofstream Debug::_log("/dev/null");
std::vector<std::string> Debug::_msgs;
std::vector<std::string>::size_type Debug::_max_msgs = 32;
const std::string Debug::_msg_info(" *INFO* ");
const std::string Debug::_msg_warn(" *WARNING* ");
const std::string Debug::_msg_err(" *ERROR* ");
const std::string Debug::_msg_bt(" *BACKTRACE* ");
unsigned int DebugFuncCall::_depth=0;
