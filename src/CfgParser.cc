//
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParser.hh"
#include "Util.hh"

#include <iostream>
#include <memory>
#include <cassert>

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

const string CfgParser::m_o_root_source_name = string("");
const char *CP_PARSE_BLANKS = " \t\n";

//! @brief CfgParser::Entry constructor.
CfgParser::Entry::Entry(const std::string &or_source_name, int i_line,
                        const std::string &or_name, const std::string &or_value)
    : m_op_entry_next(NULL), m_op_section(NULL),
      m_o_name(or_name), m_o_value(or_value),
      m_i_line(i_line), m_or_source_name(or_source_name)
{
}

//! @brief CfgParser::Entry destructor.
CfgParser::Entry::~Entry(void)
{
}

//! @brief Adds Entry to the end of Entry list at current depth.
CfgParser::Entry*
CfgParser::Entry::add_entry(const std::string &or_source_name, int i_line,
                            const std::string &or_name,
                            const std::string &or_value)
{
    if (m_op_entry_next) {
        return m_op_entry_next->add_entry(or_source_name, i_line,
                                          or_name, or_value);
    } else {
        m_op_entry_next = new Entry(or_source_name, i_line,
                                    or_name, or_value);
        return m_op_entry_next;
    }
}

//! @brief Gets next entry that has a sub section.
CfgParser::Entry*
CfgParser::Entry::get_section_next (void)
{
    Entry *op_entry;

    for (op_entry = m_op_entry_next; op_entry;
         op_entry = op_entry->m_op_entry_next) {
        if (op_entry->m_op_section) {
            return op_entry;
        }
    }

    return NULL;
}

//! @brief Gets next entry without subsection matching the name or_name.
//! @param or_name Name of Entry to look for.
CfgParser::Entry*
CfgParser::Entry::find_entry (const std::string &or_name)
{
    CfgParser::Entry *op_it;

    for (op_it = m_op_entry_next; op_it; op_it = op_it->m_op_entry_next) {
        if (!op_it->m_op_section && (*op_it == or_name.c_str ())) {
            return op_it;
        }
    }

    return NULL;
}

//! @brief Gets the next entry with subsection matchin the name or_name.
//! @param or_name Name of Entry to look for.
CfgParser::Entry*
CfgParser::Entry::find_section (const std::string &or_name)
{
    CfgParser::Entry *op_it;

    for (op_it = m_op_entry_next; op_it; op_it = op_it->m_op_entry_next) {
        if (op_it->m_op_section && (*op_it == or_name.c_str ())) {
            return op_it;
        }
    }

    return NULL;
}


//! @brief Sets and validates data specified by key list.
void
CfgParser::Entry::parse_key_values (std::list<CfgParserKey*>::iterator o_begin,
                                    std::list<CfgParserKey*>::iterator o_end)
{
    CfgParser::Entry *op_value;
    list<CfgParserKey*>::iterator o_it;

    for (o_it = o_begin; o_it != o_end; ++o_it) {
        op_value = find_entry ((*o_it)->get_name());
        if (op_value) {
            try {
                (*o_it)->parse_value(op_value->get_value());

            } catch (string &ex) {
                cerr << " *** WARNING " << ex << endl << "  "
                     << *op_value << endl;
            }
        }
    }
}


//! @brief Prints Entry tree.
void
CfgParser::Entry::print_tree(int level)
{
    Entry *op_entry;

    cerr << level << ": " << m_or_source_name << "@" << m_i_line << " |"
         << m_o_name << "| = |" << m_o_value << "| SECTION" << endl;

    for (op_entry = m_op_entry_next; op_entry;
         op_entry = op_entry->m_op_entry_next) {
        if (op_entry->m_op_section) {
            op_entry->m_op_section->print_tree(level + 1);

        } else {
            cerr << level << ": " << op_entry->m_or_source_name << "@"
                 << op_entry->m_i_line << "\t |"
                 << op_entry->m_o_name << "| = |"
                 << op_entry->m_o_value << "|" << endl;
        }
    }
}

//! @brief Frees Entry tree.
void
CfgParser::Entry::free_tree (void)
{
    Entry *op_entry, *op_entry_free;

    for (op_entry = m_op_entry_next; op_entry; ) {
        if (op_entry->m_op_section) {
            op_entry->m_op_section->free_tree();
        }

        op_entry_free = op_entry;
        op_entry = op_entry->m_op_entry_next;
        delete op_entry_free;
    }
}

//! @brief Operator <<, return info on source, line, name and value.
std::ostream&
operator<<(std::ostream &or_stream, const CfgParser::Entry &or_entry)
{
    or_stream << or_entry.get_source_name() << "@" << or_entry.get_line()
              << " " << or_entry.get_name() << " = " << or_entry.get_value();
    return or_stream;
}

//! @brief CfgParser constructor.
CfgParser::CfgParser (void)
    : m_op_source(NULL),
      m_o_root_entry(m_o_root_source_name, 0, "ROOT", ""),
      m_op_entry(&m_o_root_entry)
{
}

//! @brief CfgParser destructor.
CfgParser::~CfgParser(void)
{
    m_o_root_entry.free_tree();
}

//! @brief Parses source and fills root section with data.
bool
CfgParser::parse(const std::string &or_src, CfgParserSource::Type i_type)
{
    // Init parse buffer and reserve memory.
    string o_buf, o_value;
    o_buf.reserve(PARSE_BUF_SIZE);

    // Open initial source.
    parse_source_new(or_src, i_type);
    if (m_o_source_list.size() == 0) {
        return false;
    }

    int i_c, i_next;
    while (m_o_source_list.size()) {
        m_op_source = m_o_source_list.back();

        while ((i_c = m_op_source->getc()) != EOF) {
            switch (i_c) {
            case '\n':
                // To be able to handle entry ends AND { after \n a check
                // to see what comes after the newline is done. If { appears
                // we continue as nothing happened else we finish the entry.
                i_next = parse_skip_blank(m_op_source);
                if (i_next != '{') {
                    parse_entry_finish(o_buf, o_value);
                }
                break;
            case ';':
                parse_entry_finish(o_buf, o_value);
                break;
            case '{':
                if (parse_name(o_buf)) {
                    parse_section_finish (o_buf, o_value);
                } else {
                    cerr << "EMPTY SECTION NAME!" << endl;
                }
                o_buf.clear();
                o_value.clear();
                break;
            case '}':
                if (m_o_entry_list.size() > 0) {
                    if (o_buf.size() && parse_name(o_buf)) {
                        parse_entry_finish(o_buf, o_value);
                        o_buf.clear();
                        o_value.clear();
                    }
                    m_op_entry = m_o_entry_list.back();
                    m_o_entry_list.pop_back();
                } else {
                    cerr << "EXTRA } found!";
                }
                break;
            case '=':
                o_value.clear();
                parse_value(m_op_source, o_value);
                break;
            case '#':
                parse_comment_line(m_op_source);
                break;
            case '/':
                i_next = m_op_source->getc();
                if (i_next == '/') {
                    parse_comment_line(m_op_source);
                } else if (i_next == '*') {
                    parse_comment_c(m_op_source);
                } else {
                    o_buf += i_c;
                    m_op_source->ungetc(i_next);
                }
                break;
            default:
                o_buf += i_c;
                break;
            }
        }

        try {
            m_op_source->close();

        } catch (string &ex) {
            cerr << ex << endl;
        }
        delete m_op_source;
        m_o_source_list.pop_back();
        m_o_source_name_list.pop_back();
    }

    if (o_buf.size()) {
        parse_entry_finish(o_buf, o_value);
    }

    return true;
}

//! @brief Creates and opens new CfgParserSource.
void
CfgParser::parse_source_new(const std::string &or_name,
                            CfgParserSource::Type i_type)
{
    int i_done = 0;
    string o_name(or_name);

    do {
        CfgParserSource *op_source = source_new(o_name, i_type);
        assert(op_source);

        // Open and set as active, delete if fails.
        try {
            op_source->open();
            m_op_source = op_source;
            m_o_source_list.push_back(m_op_source);

        } catch (string &ex) {
            delete op_source;
            // Previously added in source_new
            m_o_source_name_list.pop_back();


            // Display error message on second try
            if (i_done) {
                cerr << ex << endl;
            }

            // If the open fails and we are trying to open a file, try
            // to open the file from the current files directory.
            if (!i_done && (i_type == CfgParserSource::SOURCE_FILE)) {
                if (m_o_source_name_list.size() && (or_name[0] != '/')) {
                    o_name = Util::getDir(m_o_source_name_list.back());
                    o_name += "/" + or_name;
                }
            }
        }
    } while (!i_done++ && (i_type == CfgParserSource::SOURCE_FILE));
}

//! @brief Parses from beginning to first blank.
bool
CfgParser::parse_name(std::string &or_buf)
{
    if (!or_buf.size()) {
        cerr << "EMPTY NAME!" << endl;
        return false;
    }

    // Identify name.
    string::size_type o_begin, o_end;
    o_begin = or_buf.find_first_not_of(CP_PARSE_BLANKS);
    if (o_begin == string::npos) {
        // FIXME: cerr << "ONLY BLANKS IN NAME" << endl;
        return false;
    }
    o_end = or_buf.find_first_of(CP_PARSE_BLANKS, o_begin);

    // Check if there is any garbage after the value.
    if (o_end != string::npos) {
        if (or_buf.find_first_not_of(CP_PARSE_BLANKS, o_end) != string::npos) {
            //cerr << "GARBAGE AFTER NAME" << endl;
        }
    }

    // Set name.
    or_buf = or_buf.substr(o_begin, o_end - o_begin);

    return true;
}

//! @brief Parses from = to end of " pair.
bool
CfgParser::parse_value(CfgParserSource *op_source, std::string &or_value)
{
    // Init parse buffer and reserve memory.
    string o_buf;
    o_buf.reserve(PARSE_BUF_SIZE);

    // We expect to get a " after the =, however to do proper error reporting
    // we store the what we get between so we can show the output if it includes
    // anything else than spaces.
    int i_c, i_next;
    bool b_garbage = false;
    while ((i_c = op_source->getc()) != EOF) {
        if (i_c == '"')
            break;

        o_buf += i_c;

        if (!isspace(i_c))
            b_garbage = true;
    }

    // Check if we got to a " or found EOF first.
    if (i_c == EOF) {
        cerr << "EOF before \"!" << endl;
        return false;
    }

    // Check if there was garbage between = and ".
    if (b_garbage) {
        //cerr << "GARBAGE between = and \", \"" << o_buf << "\"!" << endl;
    }

    // Parse until next ", and escape characters after \.
    o_buf.clear();
    while ((i_c = op_source->getc()) != EOF) {
        // Escape character after \, if newline drop it.
        if (i_c == '\\') {
            i_next = op_source->getc();
            if (i_next != '\n')
                o_buf += i_next;
        } else if (i_c == '"') {
            break;
        } else {
            o_buf += i_c;
        }
    }

    if (i_c == EOF) {
        cerr << "EOF reached before closing \"" << endl;
    }

    or_value = o_buf;

    return false;
}

//! @brief Parses entry (name + value) and executes command accordingly.
void
CfgParser::parse_entry_finish(std::string &or_buf, std::string &or_value)
{
    if (!or_value.size()) {
        or_buf.clear();
        return;
    }

    if (parse_name(or_buf)) {
        if (or_buf[0] == '$') {
            variable_define(or_buf, or_value);

        } else  {
            variable_expand(or_value);

            if (or_buf == "INCLUDE")  {
                parse_source_new(or_value, CfgParserSource::SOURCE_FILE);
            } else if (or_buf == "COMMAND") {
                parse_source_new(or_value, CfgParserSource::SOURCE_COMMAND);
            } else {
                m_op_entry = m_op_entry->add_entry(m_op_source->get_name(),
                                                   m_op_source->get_line(),
                                                   or_buf, or_value);
            }
        }
    } else {
        cerr << "DROPPING " << or_value << " AS IT HAS NO NAME!" << endl;
    }

    or_value.clear();
    or_buf.clear();
}

//! @brief Creates new Section on {.
void
CfgParser::parse_section_finish(std::string &or_buf, std::string &or_value)
{
    m_op_entry = m_op_entry->add_entry(m_op_source->get_name(),
                                       m_op_source->get_line(),
                                       or_buf, or_value);
    m_o_entry_list.push_back(m_op_entry);

    // Create Entry representing Section and point to it.
    Entry *op_section = new Entry(m_op_source->get_name(),
                                  m_op_source->get_line(),
                                  or_buf, or_value);
    m_op_entry->set_section(op_section);

    // Set current Entry to newly created Section.
    m_op_entry = op_section;
}

//! @brief Parses Source until end of line discarding input.
void
CfgParser::parse_comment_line(CfgParserSource *op_source)
{
    int i_c;
    while (((i_c = op_source->getc()) != EOF) && (i_c != '\n'))
        ;

    // Give back the newline, needed for flushing value before comment
    if (i_c == '\n') {
        op_source->ungetc(i_c);
    }
}

//! @brief Parses Source until */ is found.
void
CfgParser::parse_comment_c (CfgParserSource *op_source)
{
    int i_c;
    while ((i_c = op_source->getc()) != EOF) {
        if ((i_c == '*') && (op_source->getc() == '/')) {
            break;
        }
    }

    if (i_c == EOF)  {
        cerr << "EOF reached before closing */" << endl;
    }
}

//! @brief Parses Source until next non whitespace char is found.
char
CfgParser::parse_skip_blank (CfgParserSource *op_source)
{
    int i_c;
    while (((i_c = op_source->getc()) != EOF) && isspace(i_c))
        ;
    if (i_c != EOF) {
        op_source->ungetc(i_c);
    }
    return i_c;
}

//! @brief Creates a CfgParserSource of type i_type.
CfgParserSource*
CfgParser::source_new(const std::string &or_name, CfgParserSource::Type i_type)
{
    CfgParserSource *op_source = NULL;

    // Create CfgParserSource.
    m_o_source_name_list.push_back(or_name);
    m_o_source_name_set.insert(or_name);
    switch (i_type) {
    case CfgParserSource::SOURCE_FILE:
        op_source = new CfgParserSourceFile(*m_o_source_name_set.find(or_name));
        break;
    case CfgParserSource::SOURCE_COMMAND:
        op_source = new CfgParserSourceCommand(*m_o_source_name_set.find(or_name));
        break;
    default:
        break;
    }

    return op_source;
}

//! @brief Defines a variable in the m_o_var_map/setenv.
void
CfgParser::variable_define(const std::string &or_name,
                           const std::string &or_value)
{
    m_o_var_map[or_name] = or_value;

    // If the variable begins with $_ it should update the environment aswell.
    if ((or_name.size() > 2) && (or_name[1] == '_')) {
        setenv (or_name.c_str() + 2, or_value.c_str(), 1);
    }
}

//! @brief Expands all $ variables in a string.
void
CfgParser::variable_expand(std::string &or_string)
{
    string::size_type o_begin = 0, o_end = 0;

    while ((o_begin = or_string.find_first_of('$', o_end)) != string::npos) {
        o_end = o_begin + 1;

        // Skip escaped \$
        if ((o_begin > 0) && (or_string[o_begin - 1] == '\\')) {
            continue;
        }

        for (; o_end != or_string.size(); ++o_end) {
            if ((isalnum(or_string[o_end]) == 0) && (or_string[o_end] != '_'))  {
                break;
            }
        }

        string o_var_name(or_string.substr(o_begin, o_end - o_begin));

        map<string, string>::iterator o_it(m_o_var_map.find(o_var_name));
        if (o_it != m_o_var_map.end()) {
            or_string.replace(o_begin, o_end - o_begin, o_it->second);
            o_end = o_begin + o_it->second.size();

        } else  {
            cerr << "TRYING TO USE UNDEFINED VARIABLE: " << o_var_name << endl;
        }
    }
}
