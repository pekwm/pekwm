///
// BaseConfig.hh
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//                         Alaa Abd El Fatah <alaa@linux-egypt.org>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "BaseConfig.hh"
#include "Util.hh"

#include <cstdio>
#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>
#include <map>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
}

using std::cerr;
using std::endl;
using std::string;
using std::ifstream;
using std::istream;
using std::stringstream;
using std::list;
using std::map;

void sigHandler(int signal); // belongs in WindowManager.cc

namespace Parser {

static map<string, string> vars; // map for $variables

inline void
pipeFillBuffer(stringstream& dest, const string& command)
{
	int fd[2];

	if (pipe(fd) == -1) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "pipeFillBuffer: pipe() failed" << endl;
		return;
	}

	// Remove signal handler ( Otherwise this doesn't work full time )
	struct sigaction act;
        
	act.sa_handler = SIG_DFL;
	act.sa_mask = sigset_t();
	act.sa_flags = 0;
	sigaction(SIGCHLD, &act, NULL);

	pid_t pid = fork();
	if (pid == -1) { // error
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "pipeFillBuffer: fork() failed" << endl;

	} else if (pid == 0) { // child process
		dup2(fd[1], STDOUT_FILENO);	// stdout now points to the pipe

		close(fd[0]);	// close unused filedescriptor
		close(fd[1]);	// close unused filedescriptor

		execlp("/bin/sh", "sh", "-c", command.c_str(), (char *) NULL);

		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "pipeFillBuffer: execlp() failed" << endl;

		close(STDOUT_FILENO);

		exit(1);

	} else { // parent process
		close(fd[1]);	// close unused filedescriptor

		FILE *source = fdopen(fd[0],"r");
		if (source) {
			int c;
			while (!feof(source) && !ferror(source) && ((c = fgetc(source)) != EOF))
				dest.put((char) c);

			fclose(source);
		}

		int status;
		if (waitpid(pid, &status, 0) == -1) {
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "pipeFillBuffer: waitpid failed" << endl;
		}
	}

	// Restore signal handler
	act.sa_handler = sigHandler;
	act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
	sigaction(SIGCHLD, &act, NULL);
}

inline void
updateBuffer(stringstream& dest, std::istream& source, const char delim)
{
	source.get(*dest.rdbuf(), delim);

#if ( __GNUC__ == 2 ) && ( __GNUC_MINOR__ == 96 )
	dest.seekg(0, std::ios::beg);
#endif
	dest.clear();
}

inline void
updateBuffer(stringstream& buf, const string& str){
#if (__GNUC__ == 2) && (__GNUC_MINOR__ == 95)
	buf.str("");
	buf.clear();
	buf << str;
#elif (__GNUC__ == 2) && (__GNUC_MINOR__ == 96)
	buf.str(str);
	buf.seekg(0, std::ios::beg);
#else
	buf.str(str);
#endif
	buf.clear();
}

inline void
parseComment(stringstream& buf, const string::size_type begin, istream& file)
{
	string::size_type end = buf.str().find_first_of('\n', begin);
	if (end != string::npos)
	    updateBuffer(buf, buf.str().substr(end + 1, buf.str().size() - end -1));
	else {
		updateBuffer(buf, file, '\n');
		buf.str("");
	}
}

inline void
parseName(stringstream& buf, string& name)
{
	stringstream temp_buf("");
	updateBuffer(temp_buf, buf, '=');
	temp_buf >> name;
}

inline void
parseValue(stringstream& buf, const string::size_type begin, string& value)
{
	string::size_type esc = begin + 1;
	string::size_type end = buf.str().find_first_of("\n\"", begin + 1);
	value = "";
	while ( end != string::npos && buf.str()[end-1] == '\\' ) {
	    value += buf.str().substr(esc, end - esc - 1 );
	    esc=end;
	    end = buf.str().find_first_of("\n\"", end + 1);
	}

	if (end != string::npos) {
	    value += buf.str().substr(esc, end - esc);
	    updateBuffer(buf,buf.str().substr(end+1, buf.str().size() - end -1));
	} else { //something went wrong
	    value += buf.str().substr(esc, buf.str().size() - esc);
	    buf.str("");
	}
}

inline void
parseVariables(BaseConfig::CfgItem& temp_ci) {
	string::size_type begin = 0;
	string::size_type end;

	//expand variables
	while ((begin = temp_ci.value.find_first_of('$', begin)) != string::npos) {
		if (temp_ci.value[begin - 1] == '\\' )
			temp_ci.value.erase(begin - 1,1);
		else {
			if ((end = temp_ci.value.find_first_of(" \t$", begin + 1)) ==
					string::npos)
				end = temp_ci.value.size();
			temp_ci.value.replace(begin, end - begin,
														vars[temp_ci.value.substr(begin, end - begin)]);
		}
	}

	//define variable
	if (temp_ci.name[0] == '$')
	vars[temp_ci.name] = temp_ci.value;
}

}; // end namespace Parser

bool
BaseConfig::CfgSection::load(string& filename)
{
	if (!filename.size())
		return false;
	ifstream file(filename.c_str());
	if (file) {
		load(file); // maybe we should implement this as a bool
		file.close();
		return true;
	}
	return false;
}

void
BaseConfig::CfgSection::load(istream& file)
{
	string::size_type p;
	stringstream buf("");
	string temp_val;
	static string rest_of_buf = "";
	BaseConfig::CfgItem temp_ci;

	while(!file.eof() || rest_of_buf.size()) {
		if (rest_of_buf.size()) {
			Parser::updateBuffer(buf, rest_of_buf);
			rest_of_buf = "";
		} else {
		  Parser::updateBuffer(buf,file,'{');
#if ( __GNUC__ == 2 )
			if (!buf.str().size()) {
				_sect_it = _sect_list.begin();
				Parser::vars.clear();
				return;
			}
#endif //__GNUC__
		}
		while ((p = buf.str().find_first_of("\"}#;")) != string::npos) {
			switch(buf.str()[p]) {
			case ';':
				Parser::updateBuffer(buf, buf.str().substr(p + 1, buf.str().size() - p - 1));
				break;
			case '#': // comment
				Parser::parseComment(buf, p, file);
				break;
			case '}':
				if (_name.size()) {  // close section
					_sect_it = _sect_list.begin();
					// return whats between } and { to the previous call
					rest_of_buf = buf.str().substr(p + 1, buf.str().size() - p - 1);
					return;
				} else // not end of scope this is just an extra '}'
					Parser::updateBuffer(buf, buf.str().substr(p + 1, buf.str().size() - p - 1));
				break;
			case '\"': // name/value pair
				Parser::parseName(buf, temp_ci.name);
				Parser::parseValue(buf, p, temp_ci.value);
				Parser::parseVariables(temp_ci);
				if (temp_ci.name == "INCLUDE") {
				    Util::expandFileName(temp_ci.value);
				    load(temp_ci.value);
				}  else if (temp_ci.name == "COMMAND") {
				    stringstream temp_buf("");
				    Parser::pipeFillBuffer(temp_buf, temp_ci.value);
				    load(temp_buf);
				}  else if (temp_ci.name[0] != '$')
				    _values.push_back(temp_ci);
				break;
			}
		}

		// end of section.
		if (file.peek() == '{'){
			file.ignore(); // by_pass '{'

			buf.clear();
			temp_val = "";
			buf >> temp_val;
			if (temp_val.size()) {
				_sect_list.push_back(BaseConfig::CfgSection(temp_val));
				buf.str("");
				_sect_list.back().load(file);
			} else if (!_values.empty()) {
				temp_ci = _values.back();
				_sect_list.push_back(BaseConfig::CfgSection(temp_ci.name));
				temp_ci.name = "Name";
				_sect_list.back()._values.push_back(temp_ci);
				_values.pop_back();
				buf.str("");
				_sect_list.back().load(file);
			}
		}
	}

	rest_of_buf = "";
	_sect_it = _sect_list.begin();
}

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, int &value)
{
	_it = find(_values.begin(), _values.end(), name);
	if (_it != _values.end()) {
		value = atoi(_it->value.c_str());
		_values.erase(_it);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, unsigned int &value)
{
	_it = find(_values.begin(), _values.end(), name);
	if (_it != _values.end()) {
		value = (unsigned) atoi(_it->value.c_str());
		_values.erase(_it);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, bool &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, bool &value)
{
	_it = find(_values.begin(), _values.end(), name);
	if (_it != _values.end()) {
		value = Util::isTrue(_it->value);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, string &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, string &value)
{
	_it = find(_values.begin(), _values.end(), name);
	if (_it != _values.end()) {
		value = _it->value;
		_values.erase(_it);
		return true;
	}
	return false;
}

//! @fn    bool getNextValue(string &name, string &value)
//! @brief Gets the next value from the config file.
//! @return True if any objects left in the config_list
bool
BaseConfig::CfgSection::getNextValue(string &name, string &value)
{
	if (_values.size()) {
		name = _values.front().name;
		value = _values.front().value;
		_values.pop_front();
		return true;
	}
	return false;
}

//! @fn    bool load(const string &filename)
//! @brief Parses the file, unloads allready parsed info.
//! @param filename Name of file to parse.
bool
BaseConfig::load(const string &filename)
{
	if (!filename.size())
		return false;

	ifstream file(filename.c_str());
	if (file) {
		_cfg.load(file); //maybe we should implement this as a bool
		file.close();

		return true;
	}
	return false;
}

//! @fn    bool load(const string &command)
//! @brief 
//! @param command Command to execute and parse output from.
bool
BaseConfig::loadCommand(const string &command)
{
	if (!command.size())
		return false;

	stringstream command_output("");
	Parser::pipeFillBuffer(command_output, command);
	_cfg.load(command_output);

	return true;
}
