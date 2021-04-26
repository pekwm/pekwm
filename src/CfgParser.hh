//
// Copyright (C) 2005-2020 Claes Nästénn <pekdon@gmail.com>
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

#ifndef _PEKWM_CFGPARSER_HH_
#define _PEKWM_CFGPARSER_HH_

#include "config.h"

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
    typedef std::vector<std::string> filev;
    typedef filev::iterator filev_it;
    
    TimeFiles() : mtime(0) {}

    time_t mtime;
    filev files;

    bool requireReload(const std::string &file);
    void clear() { files.clear(); mtime = 0; }
};

//! @brief Configuration file parser.
class CfgParser {
public:
    typedef std::map<std::string, std::string> var_map;
    typedef var_map::iterator var_map_it;
    typedef var_map::const_iterator var_map_cit;

    //! @brief Entry in parsed data structure.
    class Entry {
    public:
        typedef std::vector<CfgParser::Entry*>::iterator entry_it;
        typedef std::vector<CfgParser::Entry*>::const_iterator entry_cit;
        
        Entry(const std::string &source_name, int line,
              const std::string &name, const std::string &value,
              CfgParser::Entry *section=0);
        Entry(const Entry &entry);
        ~Entry(void);

        entry_cit begin(void) const { return _entries.begin(); }
        entry_cit end(void) const { return _entries.end(); }

        const std::string &getName(void) const;
        const std::string &getValue(void) const;
        int getLine(void) const;
        const std::string &getSourceName(void) const;

        Entry *addEntry(Entry *entry, bool overwrite=false);
        Entry *addEntry(const std::string &source_name, int line,
                        const std::string &name, const std::string &value,
                        CfgParser::Entry *section=0, bool overwrite=false);

        //! @brief Returns the sub section.
        Entry *getSection(void) { return _section; }
        Entry *setSection(Entry *section, bool overwrite=false);

        Entry *findEntry(const std::string &name, bool include_sections=false,
                         const char *value=0) const;
        Entry *findSection(const std::string &name, const char *value=0) const;
        void parseKeyValues(std::vector<CfgParserKey*>::const_iterator begin,
                            std::vector<CfgParserKey*>::const_iterator end) const;

        void print(uint level = 0);
        void copyTreeInto(CfgParser::Entry *from, bool overwrite=false);

        //! @brief Matches Entry name agains op_rhs.
        bool operator==(const char *rhs) {
            return (strcasecmp(rhs, _name.c_str()) == 0);
        }
        friend std::ostream &operator<<(std::ostream &stream, const CfgParser::Entry &entry);

    private:
        /** List of entries in section. */
        std::vector<CfgParser::Entry*> _entries;
        Entry *_section; /**< Sub-section of node. */

        std::string _name; /**< Name of node. */
        std::string _value; /**< Value of node. */

        int _line;
        std::string _source_name;
    };

    typedef std::map<std::string, CfgParser::Entry*> section_map;
    typedef section_map::iterator section_map_it;
    typedef section_map::const_iterator section_map_cit;

    CfgParser(void);
    ~CfgParser(void);

    TimeFiles getCfgFiles(void) const { return _cfg_files; }

    /** Returns the root Entry node. */
    Entry *getEntryRoot(void) { return _root_entry; }
    /** Return true if data parsed included dynamic content such as
        from COMMAND. */
    bool isDynamicContent(void) { return _is_dynamic_content; }

    void clear(bool realloc = true);
    bool parse(const std::string &src,
               CfgParserSource::Type type = CfgParserSource::SOURCE_FILE,
               bool overwrite = false);
    bool parse(CfgParserSource* source, bool overwrite = false);

    std::string getVar(const std::string& key) const {
        var_map_cit it = _var_map.find(key);
        return it == _var_map.end() ? "" : it->second;
    }
    void setVar(const std::string& key, const std::string& val) {
        _var_map[key] = val;
    }

private:
    bool parse(void);
    void parseSourceNew(const std::string &name, CfgParserSource::Type type);
    bool parseName(std::string &buf);
    bool parseValue(std::string &value);
    void parseEntryFinish(std::string &buf, std::string &value,
                          bool &have_value);
    void parseEntryFinishStandard(std::string &buf, std::string &value,
                                  bool &have_value);
    void parseEntryFinishTemplate(std::string &name);
    void parseSectionFinish(std::string &buf, std::string &value);
    void parseCommentLine(CfgParserSource *source);
    void parseCommentC(CfgParserSource *source);
    char parseSkipBlank(CfgParserSource *source);

    CfgParserSource *sourceNew(const std::string &name,
                               CfgParserSource::Type type);

    void variableDefine(const std::string &name, const std::string &value);
    void variableExpand(std::string &var);
    bool variableExpandName(std::string &var,
                            std::string::size_type begin,
                            std::string::size_type &end);

protected:
    /** Map of $VARS */
    var_map _var_map;

private:

    CfgParserSource *_source;

    TimeFiles _cfg_files;

    /** Vector of sources, for recursive parsing. */
    std::vector<CfgParserSource*> _sources;
    /** Vector of source names, to keep track of current source. */
    std::vector<std::string> _source_names;
    /** Set of source names, source of memory usage on long-going
        CfgParser objects. */
    std::set<std::string> _source_name_set;
    std::vector<Entry*> _sections; //!< for recursive parsing.

    /**  Map of Define = ... sections */
    section_map _section_map;

    Entry *_root_entry; /**< Root Entry. */
    /** If true, parsed data included command or similar. */
    bool _is_dynamic_content;
    Entry *_section; /**< Current section. */
    bool _overwrite; /**< Overwrite elements when appending. */

    static const std::string _root_source_name; //!< Root Entry Source Name.
};

#endif // _PEKWM_CFGPARSER_HH_
