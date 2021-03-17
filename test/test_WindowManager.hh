#include "test.hh"
#include "WindowManager.hh"

#include <utility>

extern "C" {
#include <X11/Xlib.h>
}

namespace pekwm_ctrl {
#define UNITTEST
#include "pekwm_ctrl.cc"
}

class TestWindowManager : public TestSuite,
                          public WindowManager {
public:
    TestWindowManager(void);
    virtual ~TestWindowManager(void);

    void testRecvPekwmCmd(void);
    void assertSendRecvCommand(const std::string& msg, size_t expected_size,
                               const std::string& cmd);
};

TestWindowManager::TestWindowManager(void)
    : TestSuite("WindowManager"),
      WindowManager()
{
    register_test("recvPekwmCmd",
                  std::bind(&TestWindowManager::testRecvPekwmCmd, this));
}

TestWindowManager::~TestWindowManager(void)
{
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
addEv(std::vector<XClientMessageEvent> &evs,
      const void *data, size_t size)
{
    XClientMessageEvent ev = {0};
    memcpy(ev.data.b, data, size);
    evs.push_back(ev);
    return true;
}

void
TestWindowManager::assertSendRecvCommand(const std::string& msg,
                                         size_t expected_size,
                                         const std::string& cmd)
{
    std::vector<XClientMessageEvent> evs;
    auto send_message =
        [&evs] (Window, AtomName, int,
             const void *data, size_t size) -> bool {
        return addEv(evs, data, size);
    };

    pekwm_ctrl::sendCommand(cmd, None, send_message);
    ASSERT_EQUAL(msg + " sendCommand", expected_size, evs.size());
    auto it = evs.begin();
    for (; it != evs.end(); ++it) {
        bool expected = (it + 1) == evs.end() ? true : false;
        ASSERT_EQUAL(msg + " recvPekwmCmd", expected, recvPekwmCmd(&(*it)));
    }
    ASSERT_EQUAL(msg, cmd, _pekwm_cmd_buf);
}
