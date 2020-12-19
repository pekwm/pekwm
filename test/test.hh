#ifndef _TEST_HH_
#define _TEST_HH_

#include <string>
#include <sstream>

class AssertFailed {
public:
    AssertFailed(std::string file, int line, std::string msg)
        : _file(file), _line(line), _msg(msg) {
    }

    const std::string& file(void) const { return _file; }
    const int line(void) const { return _line; }
    const std::string& msg(void) const { return _msg;}

private:
    std::string _file;
    int _line;
    std::string _msg;
};

#define ASSERT_EQUAL(tc, msg, expected, actual)                         \
    if ((expected) != (actual)) {                                       \
        std::ostringstream oss;                                         \
        oss << tc <<  " " << (msg)                                      \
            << " expected " << (expected) << " got " << (actual);       \
        throw AssertFailed(__FILE__, __LINE__, oss.str());              \
    }

#endif // _TEST_HH_
