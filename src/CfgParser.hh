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

        vector<CfgParser::Entry*>::const_iterator begin(void) { return _entries.begin(); }
        vector<CfgParser::Entry*>::const_iterator end(void) { return _entries.end(); }

        //! @brief Returns the name.
        const std::string &getName(void) const { return _name; }
        //! @brief Returns the value.
        const std::string &getValue(void) const { return _value; }
        //! @brief Returns the linenumber in the source this was parsed.
        int getLine(void) const { return _line; }
        //! @brief Returns the name of the source this was parsed.
        const std::string &getSourceName(void) const { return _source_name; }

        Entry *addEntry(Entry *entry, bool overwrite=false);
        Entry *addEntry(const std::string &source_name, int line,
                        const std::string &name, const std::string &value,
                        CfgParser::Entry *section=0, bool overwrite=false);

        //! @brief Returns the sub section.
        Entry *getSection(void) { return _section; }
        Entry *setSection(Entry *section, bool overwrite=false);

        Entry *findEntry(const std::string &name, bool include_sections=false, const char *value=0);
        Entry *findSection(const std::string &name, const char *value=0);
        void parseKeyValues(vector<CfgParserKey*>::const_iterator begin,
                            vector<CfgParserKey*>::const_iterator end);

        void print(uint level = 0);
        void copyTreeInto(CfgParser::Entry *from, bool overwrite=false);

        //! @brief Matches Entry name agains op_rhs.
        bool operator==(const char *rhs) {
            return (strcasecmp(rhs, _name.c_str()) == 0);
        }
        friend std::ostream &operator<<(std::ostream &stream, const CfgParser::Entry &entry);

    private:
        vector<CfgParser::Entry*> _entries; /**< List of entries in section. */
        Entry *_section; /**< Sub-section of node. */

        std::string _name; /**< Name of node. */
        std::string _value; /**< Value of node. */

        int _line;
        const std::string &_source_name;
    };


    typedef std::vector<CfgParser::Entry*>::const_iterator iterator;

    CfgParser(void);
    ~CfgParser(void);

    TimeFiles getCfgFiles(void) const { return _cfg_files; }

    //! @brief Returns the root Entry node.
    Entry *getEntryRoot(void) { return _root_entry; }
    /** Return true if data parsed included dynamic content such as from COMMAND. */
    bool isDynamicContent(void) { return _is_dynamic_content; }

    void clear(bool realloc = true);
    bool parse(const std::string &src, CfgParserSource::Type type = CfgParserSource::SOURCE_FILE,
               bool overwrite = false);

private:
    void parseSourceNew(const std::string &name, CfgParserSource::Type type);
    bool parseName(std::string &buf);
    void parseValue(std::string &value);
    void parseEntryFinish(std::string &buf, std::string &value);
    void parseEntryFinishStandard(std::string &buf, std::string &value);
    void parseEntryFinishTemplate(std::string &name);
    void parseSectionFinish(std::string &buf, std::string &value);
    void parseCommentLine(CfgParserSource *source);
    void parseCommentC(CfgParserSource *source);
    char parseSkipBlank(CfgParserSource *source);

    CfgParserSource *sourceNew(const std::string &name, CfgParserSource::Type type);

    void variableDefine(const std::string &name, const std::string &value);
    void variableExpand(std::string &var);
    bool variableExpandName(std::string &var,
                            std::string::size_type begin, std::string::size_type &end);

    CfgParserSource *_source;

    TimeFiles _cfg_files;

    vector<CfgParserSource*> _sources; //!< Vector of sources, for recursive parsing.
    vector<std::string> _source_names; //!< Vector of source names, to keep track of current source.
    std::set<std::string> _source_name_set; //!< Set of source names, source of memory usage on long-going CfgParser objects.
    vector<Entry*> _sections; //!< for recursive parsing.

    std::map<std::string, std::string> _var_map; //!< Map of $VARS
    std::map<std::string, CfgParser::Entry*> _section_map; //!< Map of Define = ... sections

    Entry *_root_entry; /**< Root Entry. */
    bool _is_dynamic_content; /**< If true, parsed data included command or similar. */
    Entry *_section; /**< Current section. */
    bool _overwrite; /**< Overwrite elements when appending. */

    static const std::string _root_source_name; //!< Root Entry Source Name.
};

#endif // _CFG_PARSER_HH_
