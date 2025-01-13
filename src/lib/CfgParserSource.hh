//
// Copyright (C) 2005-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CFGPARSERSOURCE_HH_
#define _PEKWM_CFGPARSERSOURCE_HH_

#include <string>
#include <cstdio>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
}

#include "Compat.hh"
#include "Os.hh"

/**
 * Base class for configuration sources defining the interface and
 * common methods.
 */
class CfgParserSource
{
public:
	/**
	 * Source type description.
	 */
	enum Type {
		SOURCE_FILE, /**< Source from filesystem accesible file. */
		SOURCE_STRING, /**< Source from memory. */
		SOURCE_COMMAND, /**< Source from command output. */
		SOURCE_VIRTUAL /**< Source base type. */
	};

	CfgParserSource(const std::string &source);
	virtual ~CfgParserSource (void);

	virtual bool open(void) = 0;
	virtual void close(void) = 0;

	virtual int get_char(void) = 0;
	virtual void unget_char(int c) {
		if (c == '\n') {
			--_line;
		}
	}

	/**< Return name of source. */
	const std::string &getName(void) const { return _name; }
	/**< Return type of source. */
	CfgParserSource::Type getType(void)  const { return _type; }
	/**< Get current line from source. */
	unsigned int getLine(void) const { return _line; }
	/**< Return true if current source is dynamic. */
	bool isDynamic(void) const { return _is_dynamic; }

protected:
	int do_get_char(int c) {
		if (c == '\n') {
			++_line;
		}
		return c;
	}

protected:
	std::string _name; /**< Name of source. */
	CfgParserSource::Type _type; /**< Type of source. */
	uint _line; /**< Line number. */
	bool _is_dynamic; /**< Set to true if source has dynamic content. */
};

class CfgParserSourceFp : public CfgParserSource
{
public:
	CfgParserSourceFp(const std::string& source)
		: CfgParserSource(source),
		  _file(nullptr)
	{
	}

	/**
	 * Gets a character from _file, increments line count if \n.
	 */
	virtual int get_char(void) {
		return do_get_char(fgetc(_file));
	}

	/**
	 * Returns a character to _op_file, decrements line count if \n.
	 */
	virtual void unget_char(int c) {
		return CfgParserSource::unget_char(ungetc(c, _file));
	}

protected:
	std::FILE *_file; /**< FILE object source is reading from. */
};

/**
 * File based configuration source, reads data from a plain file on
 * disk.
 */
class CfgParserSourceFile : public CfgParserSourceFp
{
public:
	CfgParserSourceFile(const std::string &source)
		: CfgParserSourceFp(source)
	{
		_type = SOURCE_FILE;
	}
	virtual ~CfgParserSourceFile (void) { }

	virtual bool open(void);
	virtual void close(void);
};

/**
 * String based configuration source, reads data from memory.
 */
class CfgParserSourceString : public CfgParserSource
{
public:
	CfgParserSourceString(const std::string &source,
			      const std::string &data);
	virtual ~CfgParserSourceString(void);

	virtual bool open(void);
	virtual void close(void);

	virtual int get_char(void);
	virtual void unget_char(int c);

private:
	std::string _data;
	std::string::iterator _pos;
};

/**
 * Command based configuration source, executes a commands and parses
 * the output.
 */
class CfgParserSourceCommand : public CfgParserSourceFp
{
public:
	CfgParserSourceCommand(const std::string &source, Os *os, 
			       const std::string &command_path)
		: CfgParserSourceFp(source),
		  _os(os),
		  _command_path(command_path),
		  _process(nullptr)
	{
		_type = SOURCE_COMMAND;
		_is_dynamic = true;
	}
	virtual ~CfgParserSourceCommand(void) { }

	virtual bool open(void);
	virtual void close(void);

private:
	Os *_os;
	std::string _command_path; /**< PATH override for command. */
	ChildProcess *_process; /**< Process generating output. */
	struct sigaction _sigaction; /**< sigaction for restore. */
	static unsigned int _sigaction_counter; /**< Counts open. */
};

#endif // _PEKWM_CFGPARSERSOURCE_HH_
