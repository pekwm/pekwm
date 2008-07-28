//! @file
//! @author Claes Nasten <pekdon{@}pekdon{.}net
//! @date 2005-06-15
//! @brief Configuration parser source handler.

//
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
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

    CfgParserSource (const std::string &or_source) :
            _op_file (NULL), _or_name(or_source),
            _type(SOURCE_VIRTUAL), _i_line (0)
    {
    }
    virtual ~CfgParserSource (void) { }

    //! @brief Gets a character from _op_file, increments line count if \n.
    inline int getc (void) {
        int i_c = fgetc (_op_file);
        if (i_c == '\n') {
            ++_i_line;
        }
        return i_c;
    }

    //! @brief Returns a character to _op_file, decrements line count if \n.
    inline void ungetc (int i_c)  {
        ::ungetc (i_c, _op_file);
        if (i_c == '\n') {
            --_i_line;
        }
    }

    const std::string &get_name(void) { return _or_name; }
    CfgParserSource::Type get_type(void) { return _type; }
    int get_line (void) { return _i_line; }

    virtual bool open (void) throw (std::string&) { return false; }
    virtual void close (void) throw (std::string&) { }

protected:
    FILE *_op_file;
    const std::string &_or_name;
    CfgParserSource::Type _type;
    int _i_line;
};

class CfgParserSourceFile : public CfgParserSource
{
public:
    CfgParserSourceFile (const std::string &or_source)
            : CfgParserSource (or_source)
    {
        _type = SOURCE_FILE;
    }
    virtual ~CfgParserSourceFile (void) { }

    virtual bool open (void) throw (std::string&);
    virtual void close (void) throw (std::string&);
};

class CfgParserSourceCommand : public CfgParserSource
{
public:
    CfgParserSourceCommand (const std::string &or_source)
            : CfgParserSource (or_source)
    {
        _type = SOURCE_COMMAND;
    }
    virtual ~CfgParserSourceCommand (void) { }

    virtual bool open (void) throw (std::string&);
    virtual void close (void) throw (std::string&);

private:
    pid_t _o_pid;

    struct sigaction _sigaction; //!< sigaction for restore.
    static unsigned int _sigaction_counter; //!< Counts open.
};

#endif // _CFG_PARSER_SOURCE_HH_
