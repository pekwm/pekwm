#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <utility>

extern "C" {
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
}

#define DEFAULT_TAB_WIDTH 8

// Path

class Path {
public:
	Path(const std::string& path);

	const std::string& getString() const;

	bool isDir() const;

private:
	std::string _path;
	struct stat _sb;
};

Path::Path(const std::string& path)
	: _path(path)
{
	int err = stat(_path.c_str(), &_sb);
	if (err) {
		memset(&_sb, 0, sizeof(_sb));
	}
}

const std::string&
Path::getString() const
{
	return _path;
}

bool
Path::isDir() const
{
	return (_sb.st_mode & S_IFMT) == S_IFDIR;
}

// Dir

class Dir : public std::vector<std::string> {
public:
	Dir(const std::string& path);
	Dir(const Path& path);

private:
	void populate(const std::string& path);
};

Dir::Dir(const Path& path)
{
	populate(path.getString());
}

Dir::Dir(const std::string& path)
{
	populate(path);
}

void
Dir::populate(const std::string& path)
{
	DIR *dh = opendir(path.c_str());
	if (dh != NULL) {
		struct dirent *de;
		while ((de = readdir(dh)) != NULL) {
			push_back(path + "/" + de->d_name);
		}
		closedir(dh);

		sort(begin(), end());
	}
}

// Rule

class Rule {
public:
	Rule(const char* code);
	virtual ~Rule();
	const char* code() const;
	virtual bool check(const std::string& line,
			   std::ostringstream &errMessage) const = 0;

private:
	const char* _code;
};

Rule::Rule(const char* code)
	: _code(code)
{
}

Rule::~Rule()
{
}

const char*
Rule::code() const
{
	return _code;
}

// LineLengthRule

class LineLengthRule : public Rule {
public:
	LineLengthRule(int length, int tab_width);

	bool check(const std::string& line,
		   std::ostringstream& errMessage) const;
	const char* code() const;

private:
	const int _length;
	const int _tab_width;
};

LineLengthRule::LineLengthRule(int length, int tab_width=DEFAULT_TAB_WIDTH)
	: Rule("LL"),
	  _length(length),
	  _tab_width(tab_width)
{
}

bool
LineLengthRule::check(const std::string& line,
		      std::ostringstream &errMessage) const
{
	int length = 0;
	std::string::const_iterator it(line.begin());
	for (; it != line.end(); ++it) {
		length += (*it == '\t') ? _tab_width : 1;
	}

	if (length > _length) {
		errMessage << "line length " << length << " exceeds limit "
			   << _length;
		return false;
	}
	return true;
}

// TabIndentRule

class TabIndentRule : public Rule {
public:
	TabIndentRule(int tab_width=DEFAULT_TAB_WIDTH);

	bool check(const std::string& line,
		   std::ostringstream& errMessage) const;

private:
	const int _tab_width;
};

TabIndentRule::TabIndentRule(int tab_width)
	: Rule("TI"),
	  _tab_width(tab_width)
{
}

bool
TabIndentRule::check(const std::string& line,
		     std::ostringstream& errMessage) const
{
	bool in_space = false;
	int num_space = 1;

	std::string::const_iterator it(line.begin());
	for (; it != line.end(); ++it) {
		if (in_space) {
			// in space, check for number of spaces which must be
			// less than tab width.
			if (*it == ' ') {
				num_space++;
			} else if (*it == '\t') {
				errMessage << "tab in indent after space";
				return false;
			} else {
				break;
			}

			if (num_space >= _tab_width) {
				errMessage << "space in indent exceeds limit "
					   << _tab_width;
				return false;
			}
		} else if (*it == ' ') {
			in_space = true;
		} else if (*it != '\t') {
			break;
		}
	}

	return true;
}

// WhitespaceOnlyRule

class WhitespaceOnlyRule : public Rule {
public:
	WhitespaceOnlyRule();

	bool check(const std::string& line,
		   std::ostringstream& errMessage) const;
};

WhitespaceOnlyRule::WhitespaceOnlyRule()
	: Rule("WS")
{
}

bool
WhitespaceOnlyRule::check(const std::string& line,
			  std::ostringstream& errMessage) const
{
	if (line.size() == 0) {
		// newline only, ok
		return true;
	}

	std::string::const_iterator it(line.begin());
	for (; it != line.end(); ++it) {
		if (*it != ' ' && *it != '\t' && *it != '\n') {
			// non whitespace character, ok
			return true;
		}
	}

	errMessage << "whitespace only line not allowed";
	return false;
}

// LineError

class LineError {
public:
	LineError(int line, const char* code, const std::string& msg);
	~LineError();

	int line() const;
	const std::string& code() const;
	const std::string& msg() const;

private:
	int _line;
	std::string _code;
	std::string _msg;
};

LineError::LineError(int line, const char* code, const std::string& msg)
	: _line(line),
	  _code(code),
	  _msg(msg)
{
}

LineError::~LineError()
{
}

int
LineError::line() const
{
	return _line;
}

const std::string&
LineError::code() const
{
	return _code;
}

const std::string&
LineError::msg() const
{
	return _msg;
}

// CheckResult

class CheckResult {
public:
	typedef std::vector<LineError>::const_iterator const_iterator;

	CheckResult();

	const_iterator begin() const { return _errors.begin(); }
	const_iterator end() const { return _errors.end(); }

	bool isOk();
	void addError(int lineNum, const char* code,
		      const std::string &errMessage);

private:
	std::vector<LineError> _errors;
};

CheckResult::CheckResult()
{
}

bool
CheckResult::isOk()
{
	return _errors.size() == 0;
}

void
CheckResult::addError(int lineNum, const char* code,
		      const std::string &errMessage)
{
	_errors.push_back(LineError(lineNum, code, errMessage));
}

// Check

class Check {
public:
	Check();
	~Check();

	CheckResult check(std::istream& is) const;

	void addRule(Rule *rule);

private:
	std::vector<Rule*> _rules;
};

Check::Check()
{
	addRule(new LineLengthRule(80));
	addRule(new TabIndentRule());
	addRule(new WhitespaceOnlyRule());
}

Check::~Check()
{
	std::vector<Rule*>::const_iterator it(_rules.begin());
	for (; it != _rules.end(); ++it) {
		delete *it;
	}
	_rules.clear();
}

CheckResult
Check::check(std::istream& is) const
{
	CheckResult res;
	int lineNum = 1;
	for (std::string line; std::getline(is, line); lineNum++) {
		std::vector<Rule*>::const_iterator it(_rules.begin());
		for (; it != _rules.end(); ++it) {
			std::ostringstream errMessage;
			if (! (*it)->check(line, errMessage)) {
				res.addError(lineNum, (*it)->code(),
					     errMessage.str());
			}
		}
	}
	return res;
}

void
Check::addRule(Rule *rule)
{
	_rules.push_back(rule);
}

// main

void
logDebug(const std::string& msg)
{
	std::cerr << "DEBUG:   " << msg << std::endl;
}

void
logWarning(const std::string& msg)
{
	std::cerr << "WARNING: " << msg << std::endl;
}

static bool
strEndsWith(const std::string& str, const std::string& suffix)
{
	size_t pos = str.rfind(suffix);
	return pos != std::string::npos && pos == (str.size() - suffix.size());
}

static int
checkFile(const Check& check, const Path& path)
{
	logDebug("checking " + path.getString());

	std::ifstream ifs;
	ifs.open(path.getString().c_str());
	if (! ifs.is_open()) {
		logWarning("failed to open "  + path.getString());
		return 1;
	}

	CheckResult res = check.check(ifs);
	CheckResult::const_iterator it(res.begin());
	for (; it != res.end(); ++it) {
		std::cout << path.getString() << ":" << it->line()
			  << " [" << it->code() << "] "
			  << it->msg() << std::endl;
	}
	return res.isOk() ? 0 : 1;
}

static int
checkDir(const Check& check, const Path& path, const char* ext[])
{
	int ret = 0;
	Dir dir(path);
	Dir::iterator it(dir.begin());
	for (; it != dir.end(); ++it) {
		for (int i = 0; ext[i] != 0; ++i) {
			if (strEndsWith(*it, ext[i])) {
				ret = checkFile(check, *it) || ret;
			}
		}
	}
	return ret;
}

static
void usage(const char* name, int code)
{
	std::cout << "usage: " << name << " path [path1...]" << std::endl;
	exit(code);
}

int
main(int argc, char** argv)
{
	if (argc < 2) {
		usage(argv[0], 1);
	}

	const char* ext[] = {
		".cc", ".cpp", ".C",
		".hh", ".hpp", ".H",
		0
	};

	int res = 0;
	Check check;
	for (int i = 1; i < argc; i++) {
		Path path(argv[i]);
		if (path.isDir()) {
			res = checkDir(check, path, ext) || res;
		} else {
			res = checkFile(check, path) || res;
		}
	}

	return res;
}
