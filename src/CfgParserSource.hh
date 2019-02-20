//
// Copyright © 2005-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CFG_PARSER_SOURCE_HH_
#define _CFG_PARSER_SOURCE_HH_

#include <string>
#include <cstdio>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
}

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
        SOURCE_COMMAND, /**< Source from command output. */
        SOURCE_VIRTUAL /**< Source base type. */
    };

    /**
     * CfgParserSource constructor, just set default values.
     */
    CfgParserSource(const std::string &source)
        : _file(0), _name(source),
          _type(SOURCE_VIRTUAL), _line(0), _is_dynamic(false) { }
    virtual ~CfgParserSource (void) { }

    /**
     * Gets a character from _file, increments line count if \n.
     */
    inline int getc(void) {
        int c = std::fgetc(_file);
        if (c == '\n') {
            ++_line;
        }
        return c;
    }

    /**
     * Returns a character to _op_file, decrements line count if \n.
     */
    inline void ungetc(int c)  {
        std::ungetc(c, _file);
        if (c == '\n') {
            --_line;
        }
    }

    /**< Return name of source. */
    const std::string &getName(void) const { return _name; }
    /**< return type of source. */
    CfgParserSource::Type getType(void)  const { return _type; }
    /**< Get current line from source. */
    unsigned int getLine(void) const { return _line; }
    /**< Return true if current source is dynamic. */
    bool isDynamic(void) const { return _is_dynamic; }

    virtual bool open(void) { return false; }
    virtual void close(void) { }

protected:
    std::FILE *_file; /**< FILE object source is reading from. */
    const std::string &_name; /**< Name of source. */
    CfgParserSource::Type _type; /**< Type of source. */
    unsigned int _line; /**< Line number. */
    bool _is_dynamic; /**< Set to true if source has dynamic content. */
};

/**
 * File based configuration source, reads data from a plain file on
 * disk.
 */
class CfgParserSourceFile : public CfgParserSource
{
public:
    CfgParserSourceFile (const std::string &source)
        : CfgParserSource(source)
    {
        _type = SOURCE_FILE;
    }
    virtual ~CfgParserSourceFile (void) { }

    virtual bool open(void);
    virtual void close(void);
};

/**
 * Command based configuration source, executes a commands and parses
 * the output.
 */
class CfgParserSourceCommand : public CfgParserSource
{
public:
    CfgParserSourceCommand(const std::string &source)
        : CfgParserSource (source)
    {
        _type = SOURCE_COMMAND;
        _is_dynamic = true;
    }
    virtual ~CfgParserSourceCommand(void) { }

    virtual bool open(void);
    virtual void close(void);

private:
    pid_t _pid; /**< Process id of command generating output. */
    struct sigaction _sigaction; /**< sigaction for restore. */
    static unsigned int _sigaction_counter; /**< Counts open. */
};

#endif // _CFG_PARSER_SOURCE_HH_
