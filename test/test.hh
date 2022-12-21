#ifndef _TEST_HH_
#define _TEST_HH_

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

	void format(const std::string &suite_name,
		    const std::string &name) const;

private:
	std::string _file;
	int _line;
	std::string _msg;
};

void
AssertFailed::format(const std::string &suite_name,
		     const std::string &name) const
{
	std::cout << " ERROR" << std::endl;;
	std::cout << "      " << _file << ":" << _line << " ";
	std::cout << suite_name << "::" << name << " " << _msg;
	std::cout << std::endl;
}

#define ASSERT_FAILED(msg)				\
	throw AssertFailed(__FILE__, __LINE__, (msg));

#define ASSERT_TRUE(msg, actual)			\
	if (! (actual)) {				\
		std::ostringstream __test_oss;          \
		__test_oss << (msg);                    \
		__test_oss << " expected true";         \
		__test_oss << " got " << (actual);      \
		ASSERT_FAILED(__test_oss.str())         \
			}

#define ASSERT_EQUAL(msg, expected, actual)			\
	if ((expected) != (actual)) {				\
		std::ostringstream __test_oss;			\
		__test_oss << (msg);				\
		__test_oss << " expected " << (expected);	\
		__test_oss << " got " << (actual);		\
		ASSERT_FAILED(__test_oss.str())			\
			}

#define TEST_FN(spec, test_name, F)					\
	do {								\
		try {							\
			std::cout << "  * " << test_name << "...";	\
			F;						\
			std::cout << " OK" << std::endl;		\
		} catch (const AssertFailed& ex) {			\
			ex.format(name(), test_name);			\
			status = false;					\
		}							\
	} while (0)


enum TestSpec {
	TEST_RUN
};

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

	bool test(void);

protected:
	virtual bool run_test(TestSpec spec, bool status) = 0;

private:
	std::string _name;
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

bool
TestSuite::test(void)
{
	std::cout << _name << std::endl;
	return run_test(TEST_RUN, true);
}

std::vector<TestSuite*> TestSuite::_suites;

#endif // _TEST_HH_
