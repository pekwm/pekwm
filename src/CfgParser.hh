//
// Copyright © 2005-2008 Claes Nasten <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

//
// Configuration file parser with file inclusion support and command
// output parsing support. The format being parsed:
//
// $var = "value"
// INCLUDE = "file to include"
//
// section = "name" {
//   key = "name" {
//     value = "$var"
//   }
// }
//

#ifndef _CFG_PARSER_HH_
#define _CFG_PARSER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "CfgParserKey.hh"
#include "CfgParserSource.hh"

#include <list>
#include <map>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>

//! @brief Configuration file parser.
class CfgParser {
public:
    //! @brief Entry in parsed data structure.
    class Entry {
    public:
        Entry(const std::string &source_name, int line,
              const std::string &name, const std::string &value);
        ~Entry(void);

        //! @brief Returns the name.
        const std::string &get_name(void) const { return _name; }
        //! @brief Returns the value.
        const std::string &get_value(void) const { return _value; }
        //! @brief Returns the linenumber in the source this was parsed.
        int get_line(void) const { return _line; }
        //! @brief Returns the name of the source this was parsed.
        const std::string &get_source_name(void) const { return _source_name; }

        Entry *add_entry(const std::string &source_name, int line,
                         const std::string &name, const std::string &value);

        //! @brief Returns the next Entry.
        Entry *get_entry_next(void) { return _entry_next; }
        //! @brief Returns the sub section.
        Entry *get_section(void) { return _section; }
        Entry *get_section_next(void);

        //! @brief Sets the sub section.
        void set_section(Entry *section) { _section = section; }

        Entry *find_entry(const std::string &name);
        Entry *find_section(const std::string &name);

        void parse_key_values(std::list<CfgParserKey*>::iterator begin,
                              std::list<CfgParserKey*>::iterator end);

        void print_tree(int level);
        void free_tree(void);

        //! @brief Matches Entry name agains op_rhs.
        bool operator==(const char *rhs) {
            return (strcasecmp(rhs, _name.c_str()) == 0);
        }
        friend std::ostream &operator<<(std::ostream &stream, const CfgParser::Entry &entry);

    private:
        Entry *_entry_next;
        Entry *_section;

        std::string _name;
        std::string _value;

        int _line;
        const std::string &_source_name;
    };

    CfgParser(void);
    ~CfgParser(void);

    //! @brief Returns the root Entry node.
    Entry *get_entry_root(void) { return &_root_entry; }

    bool parse(const std::string &src,
               CfgParserSource::Type type = CfgParserSource::SOURCE_FILE);


private:
    void parse_source_new(const std::string &name, CfgParserSource::Type type);
    bool parse_name(std::string &buf);
    bool parse_value(CfgParserSource *source, std::string &value);
    void parse_entry_finish(std::string &buf, std::string &value);
    void parse_section_finish(std::string &buf, std::string &value);
    void parse_comment_line(CfgParserSource *source);
    void parse_comment_c(CfgParserSource *source);
    char parse_skip_blank(CfgParserSource *source);

    CfgParserSource *source_new(const std::string &name, CfgParserSource::Type type);

    void variable_define(const std::string &name, const std::string &value);
    void variable_expand(std::string &var);

private:
    CfgParserSource *_source;

    std::list<CfgParserSource*> _source_list; //!< List of sources, for recursive parsing.
    std::list<std::string> _source_name_list; //!< List of source names, to keep track of current source.
    std::set<std::string> _source_name_set; //!< Set of source names, source of memory usage on long-going CfgParser objects.
    std::list<Entry*> _entry_list; //!< List of Entries with sections, for recursive parsing.

    std::map<std::string, std::string> _var_map; //!< Map of $VARS

    Entry _root_entry; //!< Root Entry.
    Entry *_entry; //!< Current Entry.

    static const std::string _root_source_name; //!< Root Entry Source Name.
};

#endif // _CFG_PARSER_HH_
