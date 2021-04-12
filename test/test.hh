#ifndef _TEST_HH_
#define _TEST_HH_

#include <functional>
#include <iostream>
#include <map>
#include <vector>
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

#define ASSERT_FAILED(msg) \
    throw AssertFailed(__FILE__, __LINE__, (msg));

#define ASSERT_TRUE(msg, actual)                \
    if (! (actual)) {                           \
        std::ostringstream __test_oss;          \
        __test_oss << (msg);                    \
        __test_oss << " expected true";         \
        __test_oss << " got " << (actual);      \
        ASSERT_FAILED(__test_oss.str())         \
    }

#define ASSERT_EQUAL(msg, expected, actual)                             \
    if ((expected) != (actual)) {                                       \
        std::ostringstream __test_oss;                                  \
        __test_oss << (msg);                                            \
        __test_oss << " expected " << (expected);                       \
        __test_oss << " got " << (actual);                              \
        ASSERT_FAILED(__test_oss.str())                                 \
    }

typedef std::function<void()> test_fn;

class Test {
public:
    Test(const std::string& _name, test_fn _fn);
    ~Test(void);

public:
    std::string name;
    test_fn fn;
};

Test::Test(const std::string& _name, test_fn _fn)
    : name(_name),
      fn(_fn)
{
}

Test::~Test(void)
{
}

class TestSuite {
public:
    TestSuite(const std::string& name);
    virtual ~TestSuite(void);

    static int main(int, char **)
    {
        bool status = true;

        std::vector<TestSuite*>::iterator it(_suites.begin());
        for (; it != _suites.end(); ++it ) {
            status = (*it)->test() && status;
        }

        return status ? 0 : 1;
    }

    const std::string& name() const { return _name; }

    void register_test(const std::string& name, test_fn fn);

    bool test(void);

private:
    std::string _name;
    std::vector<Test> _tests;

    static std::vector<TestSuite*> _suites;
};

TestSuite::TestSuite(const std::string& name)
    : _name(name)
{
    _suites.push_back(this);
}

TestSuite::~TestSuite(void)
{
}

void
TestSuite::register_test(const std::string& name, test_fn fn)
{
    Test test(name, fn);
    _tests.push_back(test);
}

bool
TestSuite::test(void)
{
    bool status = true;

    std::cout << _name << std::endl;
    auto it = _tests.begin();
    for (; it != _tests.end(); ++it) {
        try {
            std::cout << "  * " << it->name << "...";
            it->fn();
            std::cout << " OK" << std::endl;;
        } catch (const AssertFailed& ex) {
            std::cout << " ERROR" << std::endl;;
            std::cout << "      " << ex.file() << ":" << ex.line() << " ";
            std::cout << _name << "::" << it->name << " " << ex.msg();
            std::cout << std::endl;
            status = false;
        }
    }
    return status;
}

std::vector<TestSuite*> TestSuite::_suites;

#endif // _TEST_HH_
