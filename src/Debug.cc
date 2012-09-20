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
            _log << "--- DUMBING LOG ---" << std::endl;
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

bool Debug::enable_cerr = true;
bool Debug::enable_logfile = false;
std::ofstream Debug::_log("/dev/null");
std::vector<std::string> Debug::_msgs;
std::vector<std::string>::size_type Debug::_max_msgs = 32;
const std::string Debug::_msg_info(" *INFO* ");
const std::string Debug::_msg_warn(" *WARNING* ");
const std::string Debug::_msg_err(" *ERROR* ");
unsigned int DebugFuncCall::_depth=0;
