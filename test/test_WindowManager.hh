//
// test_WindowManager.hh for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "WindowManager.hh"

#include <utility>

extern "C" {
#include <X11/Xlib.h>
}

#define UNITTEST
#include "pekwm_ctrl.cc"

class TestWindowManager : public TestSuite,
			  public WindowManager {
public:
	TestWindowManager(void);
	virtual ~TestWindowManager(void);

	bool run_test(TestSpec spec, bool status);

	void testRecvPekwmCmd(void);
	void assertSendRecvCommand(const std::string& msg, size_t expected_size,
				   const std::string& cmd);
};

TestWindowManager::TestWindowManager(void)
	: TestSuite("WindowManager"),
	  WindowManager()
{
}

TestWindowManager::~TestWindowManager(void)
{
}

bool
TestWindowManager::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "recvPekwmCmd", testRecvPekwmCmd());
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
