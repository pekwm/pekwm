//
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
//

#include "CfgParser.hh"
#include "Util.hh"

#include <algorithm>
#include <iostream>
#include <memory>
#include <cassert>
#include <cstring>

enum {
    PARSE_BUF_SIZE = 1024
};

using std::cerr;
using std::endl;
using std::list;
using std::map;
using std::set;
using std::string;
using std::auto_ptr;

const string CfgParser::_root_source_name = string("");
const char *CP_PARSE_BLANKS = " \t\n";

//! @brief CfgParser::Entry constructor.
CfgParser::Entry::Entry(const std::string &source_name, int line,
                        const std::string &name, const std::string &value,
                        CfgParser::Entry *section)
    : _section(section),
      _name(name), _value(value),
      _line(line), _source_name(source_name)
{
}

/**
 * Copy Entry together with the content.
 */
CfgParser::Entry::Entry(const CfgParser::Entry &entry)
    : _section(0),
      _name(entry._name), _value(entry._value),
      _line(entry._line), _source_name(_source_name)
{
    list<CfgParser::Entry*>::const_iterator it(entry._entries.begin());
    for (; it != entry._entries.end(); ++it) {
        _entries.push_back(new Entry(*(*it)));
    }
    if (entry._section) {
        _section = new Entry(*entry._section);
    }
}

//! @brief CfgParser::Entry destructor.
CfgParser::Entry::~Entry(void)
{
    for_each(_entries.begin(), _entries.end(), Util::Free<CfgParser::Entry*>());

    if (_section) {
        delete _section;
        _section = 0;
    }
}

/**
 * Append Entry to the end of Entry list at current depth.
 */
CfgParser::Entry*
CfgParser::Entry::add_entry(CfgParser::Entry *entry, bool overwrite)
{
    CfgParser::Entry *entry_search = 0;
    if (overwrite) {
        entry_search = find_entry(entry->get_name(), entry->get_section() != 0);
    }

    // This is a bit awkward but to keep compatible with old
    // configuration syntax overwriting of section is only allowed
    // when the value is the same.
    if (entry_search
        && (! entry_search->get_section()
            || strcasecmp(entry->get_value().c_str(), entry_search->get_value().c_str()) == 0)) { 
        entry_search->_value = entry->get_value();
        // Clear resources used by entry
        delete entry;
        entry = entry_search;
    } else {
        _entries.push_back(entry);
    }

    return entry;
}

//! @brief Adds Entry to the end of Entry list at current depth.
CfgParser::Entry*
CfgParser::Entry::add_entry(const std::string &source_name, int line,
                            const std::string &name, const std::string &value,
                            CfgParser::Entry *section, bool overwrite)
{
    return add_entry(new Entry(source_name, line, name, value, section), overwrite);
}

/**
 * Set section, copy section entires over if overwrite.
 */
CfgParser::Entry*
CfgParser::Entry::set_section(CfgParser::Entry *section, bool overwrite)
{
    if (_section) {
        if (overwrite) {
            _section->copy_tree_into(section);
            delete section;
        } else {
            delete _section;
            _section = section;
        }
    } else {
        _section = section;
    }

    return _section;
}

//! @brief Gets next entry without subsection matching the name name.
//! @param name Name of Entry to look for.
CfgParser::Entry*
CfgParser::Entry::find_entry(const std::string &name, bool include_sections)
{
    list<CfgParser::Entry*>::iterator it(_entries.begin());
    for (; it != _entries.end(); ++it) {
        if ((include_sections || ! (*it)->get_section())
            && (*(*it) == name.c_str())) {
            return *it;
        }
    }

    return 0;
}

//! @brief Gets the next entry with subsection matchin the name name.
//! @param name Name of Entry to look for.
CfgParser::Entry*
CfgParser::Entry::find_section(const std::string &name)
{
    list<CfgParser::Entry*>::iterator it(_entries.begin());
    for (; it != _entries.end(); ++it) {
        if ((*it)->get_section() && (*(*it) == name.c_str ())) {
            return (*it)->get_section();
        }
    }

    return 0;
}


//! @brief Sets and validates data specified by key list.
void
CfgParser::Entry::parse_key_values(std::list<CfgParserKey*>::iterator begin,
                                   std::list<CfgParserKey*>::iterator end)
{
    CfgParser::Entry *value;
    list<CfgParserKey*>::iterator it;

    for (it = begin; it != end; ++it) {
        value = find_entry((*it)->get_name());
        if (value) {
            try {
                (*it)->parse_value(value->get_value());

            } catch (string &ex) {
                cerr << " *** WARNING " << ex << endl << "  " << *value << endl;
            }
        }
    }
}

/**
 * Copy tree into current entry, overwrite entries if overwrite is
 * true.
 */
void
CfgParser::Entry::copy_tree_into(CfgParser::Entry *from, bool overwrite)
{
    // Copy section
    if (from->get_section()) {
        if (_section) {
            _section->copy_tree_into(from->get_section(), overwrite);
        } else {
            _section = new Entry(*(from->get_section()));
        }
    }

    // Copy elements
    CfgParser::iterator it(from->begin());
    for (; it != from->end(); ++it) {
        if (! overwrite) {
            add_entry(new Entry(*(*it)), false);
            continue;
        }

        // Check for section, if one exists either copy into existing
        // or create new copy.
        if ((*it)->get_section()) {
            CfgParser::Entry *section = find_section((*it)->get_section()->get_name());
            if (section) {
                section->copy_tree_into((*it)->get_section(), overwrite);
            } else {
                add_entry(new Entry(*(*it)), true);
            }
        } else {
            add_entry((*it)->get_source_name(), (*it)->get_line(),
                      (*it)->get_name(), (*it)->get_value(), 0, true);
        }
    }
}

//! @brief Operator <<, return info on source, line, name and value.
std::ostream&
operator<<(std::ostream &stream, const CfgParser::Entry &entry)
{
    stream << entry.get_source_name() << "@" << entry.get_line()
           << " " << entry.get_name() << " = " << entry.get_value();
    return stream;
}

//! @brief CfgParser constructor.
CfgParser::CfgParser(void)
    : _source(0),
      _root_entry(_root_source_name, 0, "ROOT", ""),
      _section(&_root_entry), _overwrite(false)
{
}

//! @brief CfgParser destructor.
CfgParser::~CfgParser(void)
{
    map<string, CfgParser::Entry*>::iterator it(_section_map.begin());
    for (; it != _section_map.end(); ++it) {
        delete it->second;
    }
}

/**
 * Parses source and fills root section with data.
 *
 * @param src Source.
 * @param type Type of source, defaults to file.
 * @param overwrite Overwrite or append duplicate elements, defaults to false.
 */
bool
CfgParser::parse(const std::string &src, CfgParserSource::Type type, bool overwrite)
{
    // Set overwrite
    _overwrite = overwrite;

    // Init parse buffer and reserve memory.
    string buf, value;
    buf.reserve(PARSE_BUF_SIZE);

    // Open initial source.
    parse_source_new(src, type);
    if (_source_list.size() == 0) {
        return false;
    }

    int c, next;
    while (_source_list.size()) {
        _source = _source_list.back();

        while ((c = _source->getc()) != EOF) {
            switch (c) {
            case '\n':
                // To be able to handle entry ends AND { after \n a check
                // to see what comes after the newline is done. If { appears
                // we continue as nothing happened else we finish the entry.
                next = parse_skip_blank(_source);
                if (next != '{') {
                    parse_entry_finish(buf, value);
                }
                break;
            case ';':
                parse_entry_finish(buf, value);
                break;
            case '{':
                if (parse_name(buf)) {
                    parse_section_finish(buf, value);
                } else {
                    cerr << "Ignoring section as name is empty." << endl;
                }
                buf.clear();
                value.clear();
                break;
            case '}':
                if (_section_list.size() > 0) {
                    if (buf.size() && parse_name(buf)) {
                        parse_entry_finish(buf, value);
                        buf.clear();
                        value.clear();
                    }
                    _section = _section_list.back();
                    _section_list.pop_back();
                } else {
                    cerr << "Extra } character found, ignoring." << endl;
                }
                break;
            case '=':
                value.clear();
                parse_value(_source, value);
                break;
            case '#':
                parse_comment_line(_source);
                break;
            case '/':
                next = _source->getc();
                if (next == '/') {
                    parse_comment_line(_source);
                } else if (next == '*') {
                    parse_comment_c(_source);
                } else {
                    buf += c;
                    _source->ungetc(next);
                }
                break;
            default:
                buf += c;
                break;
            }
        }

        try {
            _source->close();

        } catch (string &ex) {
            cerr << ex << endl;
        }
        delete _source;
        _source_list.pop_back();
        _source_name_list.pop_back();
    }

    if (buf.size()) {
        parse_entry_finish(buf, value);
    }

    return true;
}

//! @brief Creates and opens new CfgParserSource.
void
CfgParser::parse_source_new(const std::string &name_orig, CfgParserSource::Type type)
{
    int done = 0;
    string name(name_orig);

    do {
        CfgParserSource *source = source_new(name, type);
        assert(source);

        // Open and set as active, delete if fails.
        try {
            source->open();
            _source = source;
            _source_list.push_back(_source);
            done = 1;

        } catch (string &ex) {
            delete source;
            // Previously added in source_new
            _source_name_list.pop_back();


            // Display error message on second try
            if (done) {
                cerr << ex << endl;
            }

            // If the open fails and we are trying to open a file, try
            // to open the file from the current files directory.
            if (! done && (type == CfgParserSource::SOURCE_FILE)) {
                if (_source_name_list.size() && (name[0] != '/')) {
                    name = Util::getDir(_source_name_list.back());
                    name += "/" + name_orig;
                }
            }
        }
    } while (! done++ && (type == CfgParserSource::SOURCE_FILE));
}

//! @brief Parses from beginning to first blank.
bool
CfgParser::parse_name(std::string &buf)
{
    if (! buf.size()) {
        cerr << "Unable to parse empty name." << endl;
        return false;
    }

    // Identify name.
    string::size_type begin, end;
    begin = buf.find_first_not_of(CP_PARSE_BLANKS);
    if (begin == string::npos) {
        return false;
    }
    end = buf.find_first_of(CP_PARSE_BLANKS, begin);

    // Check if there is any garbage after the value.
    if (end != string::npos) {
        if (buf.find_first_not_of(CP_PARSE_BLANKS, end) != string::npos) {
            // Pass, do notihng
        }
    }

    // Set name.
    buf = buf.substr(begin, end - begin);

    return true;
}

//! @brief Parses from = to end of " pair.
bool
CfgParser::parse_value(CfgParserSource *source, std::string &value)
{
    // Init parse buffer and reserve memory.
    string buf;
    buf.reserve(PARSE_BUF_SIZE);

    // We expect to get a " after the =, however to do proper error reporting
    // we store the what we get between so we can show the output if it includes
    // anything else than spaces.
    int c, next;
    bool garbage = false;
    while ((c = source->getc()) != EOF) {
        if (c == '"') {
            break;
        }

        buf += c;

        if (! isspace(c)) {
            garbage = true;
        }
    }

    // Check if we got to a " or found EOF first.
    if (c == EOF) {
        cerr << "Reached EOF before opening \" in value." << endl;
        return false;
    }

    // Check if there was garbage between = and ".
    if (garbage) {
        // pass, do nothing
    }

    // Parse until next ", and escape characters after \.
    buf.clear();
    while ((c = source->getc()) != EOF) {
        // Escape character after \, if newline drop it.
        if (c == '\\') {
            next = source->getc();
            if (next != '\n') {
                buf += next;
            }
        } else if (c == '"') {
            break;
        } else {
            buf += c;
        }
    }

    if (c == EOF) {
        cerr << "Reached EOF before closing \" in value." << endl;
    }

    value = buf;

    return false;
}

//! @brief Parses entry (name + value) and executes command accordingly.
void
CfgParser::parse_entry_finish(std::string &buf, std::string &value)
{
    if (value.size()) {
        parse_entry_finish_standard(buf, value);
    } else {
        // Template handling, expand or define template.
        if (buf.size() && parse_name(buf) && buf[0] == '@') {
            parse_entry_finish_template(buf);
        }
        buf.clear();
    }
}
/**
 * Finish standard entry.
 */
void
CfgParser::parse_entry_finish_standard(std::string &buf, std::string &value)
{
    if (parse_name(buf)) {
        if (buf[0] == '$') {
            variable_define(buf, value);
        } else  {
            variable_expand(value);

            if (buf == "INCLUDE")  {
                parse_source_new(value, CfgParserSource::SOURCE_FILE);
            } else if (buf == "COMMAND") {
                parse_source_new(value, CfgParserSource::SOURCE_COMMAND);
            } else {
                _section->add_entry(_source->get_name(), _source->get_line(), buf, value, 0, _overwrite);
            }
        }
    } else {
        cerr << "Dropping entry with empty name." << endl;
    }

    value.clear();
    buf.clear();
}

/**
 * Finish template entry, copy data into current section.
 */
void
CfgParser::parse_entry_finish_template(std::string &name)
{
    map<string, CfgParser::Entry*>::iterator it(_section_map.find(name.c_str() + 1));
    if (it == _section_map.end()) {
        cerr << " *** WARNING: No such template " << name << endl;
        return;
    }

    _section->copy_tree_into(it->second);
}

//! @brief Creates new Section on {
void
CfgParser::parse_section_finish(std::string &buf, std::string &value)
{
    // Create Entry representing Section
    Entry *section = 0;
    if (buf.size() == 6 && strcasecmp(buf.c_str(), "DEFINE") == 0) {
        // Look for define section, started with Define = "Name" { 
        map<string, CfgParser::Entry*>::iterator it(_section_map.find(value));
        if (it != _section_map.end()) {
            delete it->second;
            _section_map.erase(it);
        }

        section = new Entry(_source->get_name(), _source->get_line(), buf, value);
        _section_map[value] = section;
    } else {
        // Create Entry for sub-section.
        section = new Entry(_source->get_name(), _source->get_line(), buf, value);

        // Add parent section, get section from parent section as it
        // can be different from the newly created if it is not
        // overwritten.
        CfgParser::Entry *parent = _section->add_entry(_source->get_name(), _source->get_line(),
                                                       buf, value, section, _overwrite);
        section = parent->get_section();
    }

    // Set current Entry to newly created Section.
    _section_list.push_back(_section);
    _section = section;
}

//! @brief Parses Source until end of line discarding input.
void
CfgParser::parse_comment_line(CfgParserSource *source)
{
    int c;
    while (((c = source->getc()) != EOF) && (c != '\n'))
        ;

    // Give back the newline, needed for flushing value before comment
    if (c == '\n') {
        source->ungetc(c);
    }
}

//! @brief Parses Source until */ is found.
void
CfgParser::parse_comment_c(CfgParserSource *source)
{
    int c;
    while ((c = source->getc()) != EOF) {
        if ((c == '*') && (source->getc() == '/')) {
            break;
        }
    }

    if (c == EOF)  {
        cerr << "Reached EOF before closing */ in comment." << endl;
    }
}

//! @brief Parses Source until next non whitespace char is found.
char
CfgParser::parse_skip_blank (CfgParserSource *source)
{
    int c;
    while (((c = source->getc()) != EOF) && isspace(c))
        ;
    if (c != EOF) {
        source->ungetc(c);
    }
    return c;
}

//! @brief Creates a CfgParserSource of type type.
CfgParserSource*
CfgParser::source_new(const std::string &name, CfgParserSource::Type type)
{
    CfgParserSource *source = 0;

    // Create CfgParserSource.
    _source_name_list.push_back(name);
    _source_name_set.insert(name);
    switch (type) {
    case CfgParserSource::SOURCE_FILE:
        source = new CfgParserSourceFile(*_source_name_set.find(name));
        break;
    case CfgParserSource::SOURCE_COMMAND:
        source = new CfgParserSourceCommand(*_source_name_set.find(name));
        break;
    default:
        break;
    }

    return source;
}

//! @brief Defines a variable in the _var_map/setenv.
void
CfgParser::variable_define(const std::string &name, const std::string &value)
{
    _var_map[name] = value;

    // If the variable begins with $_ it should update the environment aswell.
    if ((name.size() > 2) && (name[1] == '_')) {
        setenv (name.c_str() + 2, value.c_str(), 1);
    }
}

//! @brief Expands all $ variables in a string.
void
CfgParser::variable_expand(std::string &var)
{
    string::size_type begin = 0, end = 0;

    while ((begin = var.find_first_of('$', end)) != string::npos) {
        end = begin + 1;

        // Skip escaped \$
        if ((begin > 0) && (var[begin - 1] == '\\')) {
            continue;
        }

	// Find end of variable
        for (; end != var.size(); ++end) {
            if ((isalnum(var[end]) == 0) && (var[end] != '_'))  {
                break;
            }
        }

        string var_name(var.substr(begin, end - begin));
        // If the variable starts with _ it is considered an environment
        // variable, use getenv to see if it is available
        if (var_name.size() > 2 && var_name[1] == '_') {
            char *value = getenv(var_name.c_str() + 2);
            if (value) {
                var.replace(begin, end - begin, value);
                end = begin + strlen(value);
            } else {
                cerr << "Trying to use undefined environment variable: " << var_name << endl;;
            }
        } else {
            map<string, string>::iterator it(_var_map.find(var_name));
            if (it != _var_map.end()) {
                var.replace(begin, end - begin, it->second);
                end = begin + it->second.size();
            } else  {
                cerr << "Trying to use undefined variable: " << var_name << endl;
            }
        }
    }
}
