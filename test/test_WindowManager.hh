//
// test_WindowManager.hh for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "test_Mock.hh"
#include "wm/Config.hh"
#include "wm/WindowManager.hh"

#include <utility>

extern "C" {
#include <X11/Xlib.h>
}

#define UNITTEST
#include "ctrl/pekwm_ctrl.cc"

class TestWindowManager : public TestSuite,
			  public WindowManager {
public:
	TestWindowManager(void);
	virtual ~TestWindowManager(void);

	bool run_test(TestSpec spec, bool status);

	void testRecvPekwmCmd(void);
	void assertSendRecvCommand(const std::string& msg, size_t expected_size,
				   const std::string& cmd);
	void testStartBackground();
};

TestWindowManager::TestWindowManager()
	: TestSuite("WindowManager"),
	  WindowManager(new OsMock(), true)
{
}

TestWindowManager::~TestWindowManager(void)
{
}

bool
TestWindowManager::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "recvPekwmCmd", testRecvPekwmCmd());
	TEST_FN(spec, "startBackground", testStartBackground());
	return status;
}

void
TestWindowManager::testRecvPekwmCmd(void)
{
	assertSendRecvCommand("1 message", 1, "012345678901234578");
	assertSendRecvCommand("2 messages", 2, "012345678901234578 two");
	assertSendRecvCommand("3 messages", 3,
			      "012345678901234578012345678901234578 three");
}

static bool
addEv(std::vector<XClientMessageEvent> *evs,
      const void *data, size_t size)
{
	XClientMessageEvent ev = {0};
	memcpy(ev.data.b, data, size);
	evs->push_back(ev);
	return true;
}

static bool send_message(Window, AtomName, int,
			 const void *data, size_t size, void *opaque)
{
	std::vector<XClientMessageEvent> *evs =
		reinterpret_cast<std::vector<XClientMessageEvent>* >(opaque);
	return addEv(evs, data, size);
}

void
TestWindowManager::assertSendRecvCommand(const std::string& msg,
					 size_t expected_size,
					 const std::string& cmd)
{
	std::vector<XClientMessageEvent> evs;
	sendCommand(cmd, None, send_message, reinterpret_cast<void*>(&evs));
	ASSERT_EQUAL(msg + " sendCommand", expected_size, evs.size());
	std::vector<XClientMessageEvent>::iterator it = evs.begin();
	for (; it != evs.end(); ++it) {
		bool expected = (it + 1) == evs.end() ? true : false;
		ASSERT_EQUAL(msg + " recvPekwmCmd",
			     expected, recvPekwmCmd(&(*it)));
	}
	ASSERT_EQUAL(msg, cmd, _pekwm_cmd_buf);
}

void
TestWindowManager::testStartBackground()
{
	OsMock *os = static_cast<OsMock*>(_os);

	// no running process, ensure started
	startBackground("./test", "Plain #cccccc");
	ASSERT_EQUAL("exec count", 1, os->getExec().size());
	ASSERT_EQUAL("exec cmd", BINDIR "/pekwm_bg"
		     " --load-dir ./test/backgrounds Plain #cccccc",
		     os->getExec().at(0));

	// same texture, do not start again
	startBackground("./test", "Plain #cccccc");
	ASSERT_EQUAL("exec count", 1, os->getExec().size());

	// same texture, process gone, restart
	os->getExec().clear();
	startBackground("./test", "Plain #cccccc");
	ASSERT_EQUAL("exec count", 1, os->getExec().size());

	// new texture, start
	startBackground("./test", "Plain #dddddd");
	ASSERT_EQUAL("exec count", 2, os->getExec().size());
	ASSERT_EQUAL("exec cmd", BINDIR "/pekwm_bg"
		     " --load-dir ./test/backgrounds Plain #dddddd",
		     os->getExec().at(1));

	// background disabled, stop
	int count = os->getSignalCount();
	startBackground("./test", "");
	ASSERT_EQUAL("exec count", 2, os->getExec().size());
	ASSERT_EQUAL("signal count", count + 1, os->getSignalCount());
}
