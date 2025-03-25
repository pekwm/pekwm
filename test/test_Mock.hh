//
// test_Mock.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//


#ifndef _TEST_MOCK_HH_
#define _TEST_MOCK_HH_

class OsMock : public Os {
public:
	OsMock() : Os() { }
	virtual ~OsMock() { }

	virtual bool getEnv(const std::string &key, std::string &val,
			    const std::string &val_default)
	{
		if (key == "PEKWM_CONFIG_FILE") {
			val = "../test/data/config.pekwm_sys";
			return true;
		}
		val = val_default;
		return false;
	}

	virtual bool setEnv(const std::string&, const std::string&)
	{
		return false;
	}

	virtual pid_t processExec(const std::vector<std::string>&, OsEnv*)
	{
		return -1;
	}

	virtual ChildProcess *childExec(const std::vector<std::string>&,
					int flags, OsEnv *env)
	{
		return nullptr;
	}

	virtual bool processSignal(pid_t, int)
	{
		return false;
	}

	virtual bool isProcessAlive(pid_t)
	{
		return false;
	}

};

#endif // _TEST_MOCK_HH_
