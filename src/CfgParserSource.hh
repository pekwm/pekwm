//
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
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

class CfgParserSource
{
public:
    //! @brief Source type description.
    enum Type {
        SOURCE_FILE, //!< Source from filesystem accesible file.
        SOURCE_COMMAND, //!< Source from command output.
        SOURCE_VIRTUAL //!< Source base type.
    };

    CfgParserSource(const std::string &source)
        : _file(0), _name(source),
          _type(SOURCE_VIRTUAL), _line(0)
    {
    }
    virtual ~CfgParserSource (void) { }

    //! @brief Gets a character from _file, increments line count if \n.
    inline int getc(void) {
        int c = fgetc(_file);
        if (c == '\n') {
            ++_line;
        }
        return c;
    }

    //! @brief Returns a character to _op_file, decrements line count if \n.
    inline void ungetc(int c)  {
        ::ungetc (c, _file);
        if (c == '\n') {
            --_line;
        }
    }

    const std::string &get_name(void) { return _name; }
    CfgParserSource::Type get_type(void) { return _type; }
    int get_line(void) { return _line; }

    virtual bool open(void) throw (std::string&) { return false; }
    virtual void close(void) throw (std::string&) { }

protected:
    FILE *_file;
    const std::string &_name;
    CfgParserSource::Type _type;
    int _line;
};

class CfgParserSourceFile : public CfgParserSource
{
public:
    CfgParserSourceFile (const std::string &source)
        : CfgParserSource(source)
    {
        _type = SOURCE_FILE;
    }
    virtual ~CfgParserSourceFile (void) { }

    virtual bool open(void) throw (std::string&);
    virtual void close(void) throw (std::string&);
};

class CfgParserSourceCommand : public CfgParserSource
{
public:
    CfgParserSourceCommand(const std::string &source)
        : CfgParserSource (source)
    {
        _type = SOURCE_COMMAND;
    }
    virtual ~CfgParserSourceCommand(void) { }

    virtual bool open(void) throw (std::string&);
    virtual void close(void) throw (std::string&);

private:
    pid_t _pid;

    struct sigaction _sigaction; //!< sigaction for restore.
    static unsigned int _sigaction_counter; //!< Counts open.
};

#endif // _CFG_PARSER_SOURCE_HH_
