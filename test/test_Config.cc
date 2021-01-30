#include "test.hh"
#include "Config.hh"
#include "Action.hh"

class TestConfig : public TestSuite,
                   public Config {
public:
    TestConfig()
        : TestSuite("Config"),
          Config()
    {
        register_test("parseMoveResizeAction",
                      std::bind(&TestConfig::testParseMoveResizeAction,
                                this));
    }

    void testParseMoveResizeAction(void) {
        Action action;
        ASSERT_EQUAL("parse 1 ok", true, parseMoveResizeAction("Movehorizontal 1", action));
        ASSERT_EQUAL("parsed value", 1, action.getParamI(0));
        ASSERT_EQUAL("parsed unit", UNIT_PIXEL, action.getParamI(1));

        ASSERT_EQUAL("parse -3% ok", true, parseMoveResizeAction("Movehorizontal -3%", action));
        ASSERT_EQUAL("parsed value", -3, action.getParamI(0));
        ASSERT_EQUAL("parsed unit", UNIT_PERCENT, action.getParamI(1));
    }
};

int
main(int argc, char *argv[])
{
    TestConfig testConfig;
    TestSuite::main(argc, argv);
}
