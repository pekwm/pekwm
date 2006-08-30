//! @file
//! @author Claes Nasten <pekdon{@}pekdon{.}net
//! @date 2005-05-21
//! @brief Configuration file parser.
//! Configuration file parser with file inclusion support and command
//! output parsing support.
//! The format beeing parsed:
//! $var = "value"
//! !include = "file to include"
//! section = "name" {
//!   key = "name" {
//!     value = "$var"
//!   }
//! }

//
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CFG_PARSER_HH_
#define _CFG_PARSER_HH_

#include "CfgParserKey.hh"
#include "CfgParserSource.hh"

#include <list>
#include <map>
#include <set>
#include <string>
#include <iostream>
#include <cstdlib>

//! @brief Configuration file parser.
class CfgParser {
public:
    //! @brief Entry in parsed data structure.
    class Entry {
    public:
        Entry (const std::string &or_source_name, int i_line,
               const std::string &or_name, const std::string &or_value);
        ~Entry (void);

        //! @brief Returns the name.
        const std::string &get_name (void) const { return m_o_name; }
        //! @brief Returns the value.
        const std::string &get_value (void) const { return m_o_value; }
        //! @brief Returns the linenumber in the source this was parsed.
        int get_line (void) const { return m_i_line; }
        //! @brief Returns the name of the source this was parsed.
        const std::string &get_source_name (void) const { return m_or_source_name; }

        Entry *add_entry (const std::string &or_source_name, int i_line,
                          const std::string &or_name, const std::string &or_value);

        //! @brief Returns the next Entry.
        Entry *get_entry_next (void) { return m_op_entry_next; }
        //! @brief Returns the sub section.
        Entry *get_section (void) { return m_op_section; }
        Entry *get_section_next (void);

        //! @brief Sets the sub section.
        void set_section (Entry *op_section) { m_op_section = op_section; }

        Entry *find_entry (const std::string &or_name);
        Entry *find_section (const std::string &or_name);

        void parse_key_values (std::list<CfgParserKey*>::iterator o_begin,
                               std::list<CfgParserKey*>::iterator o_end);

        void print_tree (int level);
        void free_tree (void);

        //! @brief Matches Entry name agains op_rhs.
        bool operator== (const char *op_rhs) {
            return (strcasecmp (op_rhs, m_o_name.c_str()) == 0);
        }
        friend std::ostream &operator<< (std::ostream &or_stream,
                                         const CfgParser::Entry &or_entry);

    private:
        Entry *m_op_entry_next;
        Entry *m_op_section;

        std::string m_o_name;
        std::string m_o_value;

        int m_i_line;
        const std::string &m_or_source_name;
    };

    CfgParser (void);
    ~CfgParser (void);

    //! @brief Returns the root Entry node.
    Entry *get_entry_root (void) { return &m_o_root_entry; }

    bool parse (const std::string &or_src,
                CfgParserSource::Type i_type = CfgParserSource::SOURCE_FILE);


private:
    void parse_source_new (const std::string &or_name,
                           CfgParserSource::Type i_type);
    bool parse_name (std::string &or_buf);
    bool parse_value (CfgParserSource *op_source, std::string &or_value);
    void parse_entry_finish (std::string &or_buf, std::string &or_value);
    void parse_section_finish (std::string &or_buf, std::string &or_value);
    void parse_comment_line (CfgParserSource *op_source);
    void parse_comment_c (CfgParserSource *op_source);
    char parse_skip_blank (CfgParserSource *op_source);

    CfgParserSource *source_new (const std::string &or_name,
                                 CfgParserSource::Type i_type);

    void variable_define (const std::string &or_name, const std::string &or_value);
    void variable_expand (std::string &or_string);

private:
    CfgParserSource *m_op_source;

    std::list<CfgParserSource*> m_o_source_list; //!< List of sources, for recursive parsing.
    std::list<std::string> m_o_source_name_list; //!< List of source names, to keep track of current source.
    std::set<std::string> m_o_source_name_set; //!< Set of source names, source of memory usage on long-going CfgParser objects.
    std::list<Entry*> m_o_entry_list; //!< List of Entries with sections, for recursive parsing.

    std::map<std::string, std::string> m_o_var_map; //!< Map of $VARS

    Entry m_o_root_entry; //!< Root Entry.
    Entry *m_op_entry; //!< Current Entry.

    static const std::string m_o_root_source_name; //!< Root Entry Source Name.
};

#endif // _CFG_PARSER_HH_
