//
// test_Mock.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TEST_MOCK_HH_
#define _TEST_MOCK_HH_

#include <map>
#include <string>
#include <vector>

#include "Os.hh"

class OsMock : public Os {
public:
	OsMock() :
		Os(),
		_pid(0),
		_signal_count(0)
	{
		_env["PEKWM_CONFIG_FILE"] = "../test/data/config.pekwm_sys";
	}
	virtual ~OsMock() { }

	virtual bool getEnv(const std::string &key, std::string &val,
			    const std::string &val_default)
	{
		std::map<std::string, std::string>::iterator it(_env.find(key));
		if (it == _env.end()) {
			val = val_default;
			return false;
		}
		val = it->second;
		return true;
	}

	virtual bool setEnv(const std::string& key, const std::string& val)
	{
		_env[key] = val;
		return true;
	}

	virtual bool pathExists(const std::string&) { return false; }

	virtual pid_t processExec(const std::vector<std::string>& args, OsEnv*)
	{
		std::string exec;
		std::vector<std::string>::const_iterator it(args.begin());
		for (; it != args.end(); ++it) {
			if (it != args.begin()) {
				exec += " ";
			}
			exec += *it;
		}
		_exec.push_back(exec);
		return ++_pid;
	}

	virtual ChildProcess *childExec(const std::vector<std::string>&,
					int flags, OsEnv *env)
	{
		return nullptr;
	}

	virtual bool processSignal(pid_t, int)
	{
		_signal_count++;
		return true;
	}

	virtual bool isProcessAlive(pid_t pid) { return !_exec.empty(); }

	int getSignalCount() const { return _signal_count; }
	std::vector<std::string>& getExec() { return _exec; }

private:
	std::map<std::string, std::string> _env;
	pid_t _pid;
	int _signal_count;
	std::vector<std::string> _exec;
};

#endif // _TEST_MOCK_HH_
