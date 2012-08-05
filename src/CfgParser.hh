//
// Copyright © 2005-2009 Claes Nasten <me{@}pekdon{.}net>
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

#include <vector>
#include <list>
#include <map>
#include <set>
#include <string>
#include <cstring>
#include <iostream>
#include <cstdlib>

//! @brief Helper class
class TimeFiles {
public:
    TimeFiles() : mtime(0) {}

    time_t mtime;
    std::vector<std::string> files;

    bool requireReload(const std::string &file);
    void clear() { files.clear(); mtime = 0; }
};

//! @brief Configuration file parser.
class CfgParser {
public:
    //! @brief Entry in parsed data structure.
    class Entry {
    public:
        Entry(const std::string &source_name, int line,
              const std::string &name, const std::string &value,
              CfgParser::Entry *section=0);
        Entry(const Entry &entry);
        ~Entry(void);

        std::list<CfgParser::Entry*>::iterator begin(void) { return _entries.begin(); }
        std::list<CfgParser::Entry*>::iterator end(void) { return _entries.end(); }

        //! @brief Returns the name.
        const std::string &get_name(void) const { return _name; }
        //! @brief Returns the value.
        const std::string &get_value(void) const { return _value; }
        //! @brief Returns the linenumber in the source this was parsed.
        int get_line(void) const { return _line; }
        //! @brief Returns the name of the source this was parsed.
        const std::string &get_source_name(void) const { return _source_name; }

        Entry *add_entry(Entry *entry, bool overwrite=false);
        Entry *add_entry(const std::string &source_name, int line,
                         const std::string &name, const std::string &value,
                         CfgParser::Entry *section=0, bool overwrite=false);

        //! @brief Returns the sub section.
        Entry *get_section(void) { return _section; }
        Entry *set_section(Entry *section, bool overwrite=false);

        Entry *find_entry(const std::string &name, bool include_sections=false, const char *value=0);
        Entry *find_section(const std::string &name, const char *value=0);
        void parse_key_values(std::list<CfgParserKey*>::iterator begin,
                              std::list<CfgParserKey*>::iterator end);

        void print(uint level = 0);
        void copy_tree_into(CfgParser::Entry *from, bool overwrite=false);

        //! @brief Matches Entry name agains op_rhs.
        bool operator==(const char *rhs) {
            return (strcasecmp(rhs, _name.c_str()) == 0);
        }
        friend std::ostream &operator<<(std::ostream &stream, const CfgParser::Entry &entry);

    private:
        std::list<CfgParser::Entry*> _entries; /**< List of entries in section. */
        Entry *_section; /**< Sub-section of node. */

        std::string _name; /**< Name of node. */
        std::string _value; /**< Value of node. */

        int _line;
        const std::string &_source_name;
    };


    typedef std::list<CfgParser::Entry*>::iterator iterator;

    CfgParser(void);
    ~CfgParser(void);

    /** Return map of file / mtime */
    const std::map<std::string, time_t> &get_file_list(void) const { return _file_list; }

    //! @brief Returns the root Entry node.
    Entry *get_entry_root(void) { return _root_entry; }
    /** Return true if data parsed included dynamic content such as from COMMAND. */
    bool is_dynamic_content(void) { return _is_dynamic_content; }

    void clear(bool realloc = true);
    bool parse(const std::string &src, CfgParserSource::Type type = CfgParserSource::SOURCE_FILE,
               bool overwrite = false);

private:
    void parse_source_new(const std::string &name, CfgParserSource::Type type);
    bool parse_name(std::string &buf);
    void parse_value(CfgParserSource *source, std::string &value);
    void parse_entry_finish(std::string &buf, std::string &value);
    void parse_entry_finish_standard(std::string &buf, std::string &value);
    void parse_entry_finish_template(std::string &name);
    void parse_section_finish(std::string &buf, std::string &value);
    void parse_comment_line(CfgParserSource *source);
    void parse_comment_c(CfgParserSource *source);
    char parse_skip_blank(CfgParserSource *source);

    CfgParserSource *source_new(const std::string &name, CfgParserSource::Type type);

    void variable_define(const std::string &name, const std::string &value);
    void variable_expand(std::string &var);
    bool variable_expand_name(std::string &var,
                              std::string::size_type begin, std::string::size_type &end);
private:
    CfgParserSource *_source;

    std::map<std::string, time_t> _file_list; //!< Map of source, mtime of loaded files. */

    std::list<CfgParserSource*> _source_list; //!< List of sources, for recursive parsing.
    std::list<std::string> _source_name_list; //!< List of source names, to keep track of current source.
    std::set<std::string> _source_name_set; //!< Set of source names, source of memory usage on long-going CfgParser objects.
    std::list<Entry*> _section_list; //!< List sections, for recursive parsing.

    std::map<std::string, std::string> _var_map; //!< Map of $VARS
    std::map<std::string, CfgParser::Entry*> _section_map; //!< Map of Define = ... sections

    Entry *_root_entry; /**< Root Entry. */
    bool _is_dynamic_content; /**< If true, parsed data included command or similar. */
    Entry *_section; /**< Current section. */
    bool _overwrite; /**< Overwrite elements when appending. */

    static const std::string _root_source_name; //!< Root Entry Source Name.
};

#endif // _CFG_PARSER_HH_
