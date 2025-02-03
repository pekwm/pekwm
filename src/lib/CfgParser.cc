//
// Copyright (C) 2005-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParser.hh"
#include "Debug.hh"
#include "Compat.hh"
#include "String.hh"

#include <algorithm>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

const std::string CfgParser::_root_source_name = std::string("");
const char *CP_PARSE_BLANKS = " \t\n";

bool
TimeFiles::requireReload(const std::string &file)
{
	// Check for the file, signal reload if not previously loaded.
	filev_it it = find(files.begin(), files.end(), file);
	if (it == files.end()) {
		return true;
	}

	struct stat stat_buf;
	// Check state of all files, if one is updated reload.
	for (it = files.begin(); it != files.end(); ++it) {
		if (stat((*it).c_str(), &stat_buf))
			stat_buf.st_mtime = 0;

		if (stat_buf.st_mtime > mtime) {
			if (time(0) > stat_buf.st_mtime) {
				mtime = stat_buf.st_mtime;
			}
			return true;
		}
	}

	return false;
}

//! @brief CfgParser::Entry constructor.
CfgParser::Entry::Entry(const std::string &source_name, int line,
			const std::string &name, const std::string &value,
			CfgParser::Entry *section)
	: _section(section),
	  _name(name),
	  _value(value),
	  _line(line),
	  _source_name(source_name)
{
}

/**
 * Copy Entry together with the content.
 */
CfgParser::Entry::Entry(const CfgParser::Entry &entry)
	: _section(0),
	  _name(entry._name),
	  _value(entry._value),
	  _line(entry._line),
	  _source_name(entry._source_name)
{
	entry_cit it = entry.begin();
	for (; it != entry.end(); ++it) {
		_entries.push_back(new Entry(*(*it)));
	}
	if (entry._section) {
		_section = new Entry(*entry._section);
	}
}

CfgParser::Entry::~Entry(void)
{
	for (entry_it it = _entries.begin(); it != _entries.end(); ++it) {
		delete *it;
	}
	delete _section;
}

const std::string&
CfgParser::Entry::getName(void) const
{
	return _name;
}

const std::string&
CfgParser::Entry::getValue(void) const
{
	return _value;
}

int
CfgParser::Entry::getLine(void) const
{
	return _line;
}

const std::string&
CfgParser::Entry::getSourceName(void) const
{
	return _source_name;
}

/**
 * Append Entry to the end of Entry list at current depth.
 */
CfgParser::Entry*
CfgParser::Entry::addEntry(CfgParser::Entry *entry, bool overwrite)
{
	CfgParser::Entry *entry_search = 0;
	if (overwrite) {
		if (entry->getSection()) {
			const char* name =
				entry->getSection()->getValue().c_str();
			entry_search = findEntry(entry->getName(), true, name);
		} else {
			entry_search = findEntry(entry->getName(), false);
		}
	}

	// This is a bit awkward but to keep compatible with old
	// configuration syntax overwriting of section is only allowed
	// when the value is the same.
	if (entry_search
	    && (! entry_search->getSection()
		|| pekwm::ascii_ncase_equal(entry->getValue(),
					    entry_search->getValue()))) {
		entry_search->_value = entry->getValue();
		entry_search->setSection(entry->getSection(), overwrite);

		// Clear resources used by entry
		entry->_section = 0;
		delete entry;
		entry = entry_search;
	} else {
		_entries.push_back(entry);
	}

	return entry;
}

//! @brief Adds Entry to the end of Entry list at current depth.
CfgParser::Entry*
CfgParser::Entry::addEntry(const std::string &source_name, int line,
			   const std::string &name, const std::string &value,
			   CfgParser::Entry *section, bool overwrite)
{
	return addEntry(new Entry(source_name, line, name, value, section),
			overwrite);
}

/**
 * Set section, copy section entires over if overwrite.
 */
CfgParser::Entry*
CfgParser::Entry::setSection(CfgParser::Entry *section, bool overwrite)
{
	if (_section) {
		if (overwrite) {
			_section->copyTreeInto(section, overwrite);
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
CfgParser::Entry::findEntry(const std::string &name, bool include_sections,
			    const char *value) const
{
	for (entry_cit it = begin(); it != end(); ++it) {
		CfgParser::Entry *value_check =
			include_sections ? (*it)->getSection() : *it;

		if (*(*it) == name.c_str()
		    && (! (*it)->getSection() || include_sections)
		    && (! value || (value_check
				    && value_check->getValue() == value))) {
			return *it;
		}
	}

	return 0;
}

//! @brief Gets the next entry with subsection matchin the name name.
//! @param name Name of Entry to look for.
CfgParser::Entry*
CfgParser::Entry::findSection(const std::string &name, const char *value) const
{
	entry_cit it = begin();
	for (; it != end(); ++it) {
		if ((*it)->getSection() && *(*it) == name.c_str()
		    && (! value || (*it)->getSection()->getValue() == value)) {
			return (*it)->getSection();
		}
	}

	return nullptr;
}


//! @brief Sets and validates data specified by key list.
void
CfgParser::Entry::parseKeyValues(std::vector<CfgParserKey*>::const_iterator it,
				 std::vector<CfgParserKey*>::const_iterator
				     end)
	const
{
	for (; it != end; ++it) {
		CfgParser::Entry *value = findEntry((*it)->getName());
		try {
			if (value) {
				(*it)->parseValue(value->getValue(), false);
			} else {
				(*it)->parseValue("", true);
			}
		} catch (std::string &ex) {
			P_WARN("parsing value " << *value << " failed with "
			       "exception: " << ex);
		}
	}
}

/**
   Print tree to stderr.
*/
void
CfgParser::Entry::print(uint level)
{
	for (uint i = 0; i < level; ++i) {
		std::cerr << " ";
	}
	std::cerr << " * " << getName() << "=" << getValue() << std::endl;

	CfgParser::Entry::entry_cit it(begin());
	for (; it != end(); ++it) {
		if ((*it)->getSection()) {
			(*it)->getSection()->print(level + 1);
		} else {
			for (uint i = 0; i < level; ++i) {
				std::cerr << " ";
			}
			std::cerr << "   - " << (*it)->getName() << "="
				  << (*it)->getValue() << std::endl;
		}
	}
}

/**
 * Copy tree into current entry, overwrite entries if overwrite is
 * true.
 */
void
CfgParser::Entry::copyTreeInto(CfgParser::Entry *from, bool overwrite)
{
	// Copy section
	if (from->getSection()) {
		if (_section) {
			_section->copyTreeInto(from->getSection(), overwrite);
		} else {
			_section = new Entry(*(from->getSection()));
		}
	}

	// Copy elements
	CfgParser::Entry::entry_cit it(from->begin());
	for (; it != from->end(); ++it) {
		CfgParser::Entry *entry_section = 0;
		if ((*it)->getSection()) {
			entry_section = new Entry(*((*it)->getSection()));
		}
		addEntry((*it)->getSourceName(), (*it)->getLine(),
			 (*it)->getName(), (*it)->getValue(),
			 entry_section, true);
	}
}

//! @brief Operator <<, return info on source, line, name and value.
std::ostream&
operator<<(std::ostream &stream, const CfgParser::Entry &entry)
{
	stream << entry.getSourceName() << "@" << entry.getLine();
	stream << " " << entry.getName() << " = " << entry.getValue();
	return stream;
}

CfgParser::CfgParser(const CfgParserOpt &opt)
	: _opt(opt),
	  _os(opt.getOs() == nullptr ? mkOs() : opt.getOs()),
	  _source(nullptr),
	  _root_entry(nullptr),
	  _is_dynamic_content(false),
	  _is_end_early(false),
	  _section(_root_entry),
	  _overwrite(false)
{
	CfgParserVarExpanderType types[] = {
		CFG_PARSER_VAR_EXPANDER_OS_ENV,
		CFG_PARSER_VAR_EXPANDER_X11_ATOM,
		CFG_PARSER_VAR_EXPANDER_X11_RES
	};
	for (size_t i = 0; i < sizeof(types)/sizeof(types[0]); i++) {
		CfgParserVarExpander* exp =
			mkCfgParserVarExpander(types[i],
					       _opt.registerXResource());
		if (exp != nullptr) {
			_var_expanders.push_back(exp);
		}
	}

	// memory expander is put last as it does not require a
	// specific prefix to lookup and would lookup all vars if
	// placed first.
	_var_expander_mem = mkCfgParserVarExpander(CFG_PARSER_VAR_EXPANDER_MEM,
						   _opt.registerXResource());
	_var_expanders.push_back(_var_expander_mem);

	_root_entry = new CfgParser::Entry(_root_source_name, 0, "ROOT", "");
	_section = _root_entry;
}

//! @brief CfgParser destructor.
CfgParser::~CfgParser(void)
{
	clear(false);

	std::vector<CfgParserVarExpander*>::iterator it(_var_expanders.begin());
	for (; it != _var_expanders.end(); ++it) {
		delete *it;
	}

	if (! _opt.getOs()) {
		delete _os;
	}
}

/**
 * Clear resources used by parser, end up in the same state as in
 * after construction.
 *
 * @param realloc If realloc is false, root_entry will be cleared as
 *        well rendering the parser useless. Defaults to true.
 */
void
CfgParser::clear(bool realloc)
{
	_source = nullptr;
	delete _root_entry;
	_is_dynamic_content = false;
	_is_end_early = false;

	if (realloc) {
		_root_entry = new CfgParser::Entry(_root_source_name, 0,
						   "ROOT", "");
	} else {
		_root_entry = 0;
	}

	_section = _root_entry;
	_overwrite = false;

	// Clear lists
	_sources.clear();
	_source_names.clear();
	_source_name_set.clear();
	_sections.clear();
	_var_expander_mem->clear();

	// Remove sections
	section_map_it it = _section_map.begin();
	for (; it  != _section_map.end(); ++it) {
		delete it->second;
	}
	_section_map.clear();
}

/**
 * Parses source and fills root section with data.
 *
 * @param src Source.
 * @param type Type of source, defaults to file.
 * @param overwrite Overwrite or append duplicate elements, defaults to false.
 */
bool
CfgParser::parse(const std::string &src, CfgParserSource::Type type,
		 bool overwrite)
{
	// Set overwrite
	_overwrite = overwrite;

	// Open initial source.
	parseSourceNew(src, type);
	if (_sources.size() == 0) {
		return false;
	}
	return parse();
}

/**
 * Parses source and fills root section with data.
 *
 * @param source Source.
 * @param type Type of source, defaults to file.
 * @param overwrite Overwrite or append duplicate elements, defaults to false.
 */
bool
CfgParser::parse(CfgParserSource* source, bool overwrite)
{
	_overwrite = overwrite;
	_source = source;
	_sources.push_back(source);
	_source_names.push_back(source->getName());
	_source_name_set.insert(source->getName());
	return parse();
}

bool
CfgParser::parse()
{
	CfgParserState ps;
	while (! _is_end_early && _sources.size()) {
		_source = _sources.back();
		parseSource(_source, ps);
		try {
			_source->close();
		} catch (std::string &ex) {
			P_LOG("failed to close source, raised exception: "
			      << ex);
		}

		// source might change in parseEntryFinish
		CfgParserSource* source = _source;
		_sources.pop_back();
		_source_names.pop_back();

		// done inside of the loop to ensure COMMAND and INCLUDE
		// statements without a new line at the end of the file will
		// be used before deleting the source.
		if (ps.buf.size()) {
			parseEntryFinish(ps);
		}

		delete source;
	}

	while (! _sources.empty()) {
		delete _sources.back();
		_sources.pop_back();
		_source_names.pop_back();
	}
	_sections.clear();

	return true;
}

/**
 * Creates and opens new CfgParserSource.
 */
void
CfgParser::parseSourceNew(const std::string &name_orig,
			  CfgParserSource::Type type)
{
	int done = 0;
	std::string name(name_orig);

	do {
		CfgParserSource *source = sourceNew(name, type);
		if (! source) {
			P_ERR("source == 0: " << name);
			return;
		}

		// Open and set as active, delete if fails.
		try {
			source->open();
			// Add source to file list if file
			if (type == CfgParserSource::SOURCE_FILE) {
				time_t time = Util::getMtime(name);
				_cfg_files.files.push_back(name);
				if (_cfg_files.mtime < time) {
					_cfg_files.mtime = time;
				}
			}

			_source = source;
			_sources.push_back(_source);
			done = 1;

		} catch (std::string &ex) {
			delete source;
			// Previously added in source_new
			_source_names.pop_back();

			// Display error message on second try
			if (done) {
				USER_WARN(ex);
			}

			// If the open fails and we are trying to open a file,
			// try to open the file from the current files
			// directory.
			if (! done && (type == CfgParserSource::SOURCE_FILE)) {
				if (_source_names.size() && name[0] != '/') {
					name = Util::getDir(
							_source_names.back());
					name += "/" + name_orig;
				}
			}
		}
	} while (! done++ && (type == CfgParserSource::SOURCE_FILE));
}

void
CfgParser::parseSource(CfgParserSource *source, CfgParserState &ps)
{
	if (_source->isDynamic()) {
		_is_dynamic_content = true;
	}

	int c;
	while (! _is_end_early && (c = _source->get_char()) != EOF) {
		switch (c) {
		case '\n': {
			// To be able to handle entry ends AND { after
			// \n a check to see what comes after the
			// newline is done. If { appears we continue as
			// nothing happened else we finish the entry.
			int next = parseSkipBlank(_source);
			if (next != '{') {
				parseEntryFinish(ps);
			}
			break;
		}
		case ';':
			parseEntryFinish(ps);
			break;
		case '{':
			if (parseName(ps.buf)) {
				parseSectionFinish(ps);
			} else {
				USER_WARN("Ignoring section as name "
					  "is empty.");
			}
			ps.reset();
			break;
		case '}':
			parseCloseCurlyBracket(ps);
			break;
		case '=':
			ps.value = "";
			ps.have_value = parseValue(ps.value);
			break;
		case '#':
			parseCommentLine(_source);
			break;
		case '/': {
			int next = _source->get_char();
			if (next == '/') {
				parseCommentLine(_source);
			} else if (next == '*') {
				parseCommentC(_source);
			} else {
				ps.buf += c;
				_source->unget_char(next);
			}
			break;
		}
		default:
			ps.buf += c;
			break;
		}
	}
}

//! @brief Parses from beginning to first blank.
bool
CfgParser::parseName(std::string &buf)
{
	if (! buf.size()) {
		USER_WARN("Unable to parse empty name.");
		return false;
	}

	// Identify name.
	size_t begin = buf.find_first_not_of(CP_PARSE_BLANKS);
	if (begin == std::string::npos) {
		return false;
	}

	if (buf[begin] == '"') {
		// quoted name
		std::string name;
		bool in_escape = false;
		for (size_t i = begin + 1; i < buf.size(); i++) {
			if (in_escape) {
				name += buf[i];
				in_escape = false;
			} else if (buf[i] == '\\') {
				in_escape = true;
			} else if (buf[i] == '"') {
				break;
			} else {
				name += buf[i];
			}
		}
		buf = name;
	} else {
		size_t end = buf.find_first_of(CP_PARSE_BLANKS, begin);
		buf = buf.substr(begin, end - begin);
	}

	return true;
}

/**
 * Parse _source after = to end of " pair.
 */
bool
CfgParser::parseValue(std::string &value)
{
	// Expect to get a " after the =, ignore anything else.
	int c;
	while ((c = _source->get_char()) != EOF && c != '"') {
		;
	}

	// Check if EOF before getting a quotation mark.
	if (c == EOF) {
		USER_WARN("Reached EOF before opening \" in value.");
		return false;
	}

	// Parse until next ", and escape characters after \.
	while ((c = _source->get_char()) != EOF && c != '"') {
		// Escape character after \, if newline drop it.
		if (c == '\\') {
			c = _source->get_char();
			if (c == '\n' || c == EOF) {
				continue;
			}
		}
		value += c;
	}

	P_LOG_IF(c == EOF, "Reached EOF before closing \" in value.");
	return c != EOF;
}

/**
 * Parses entry (name + value) and executes command accordingly.
 */
void
CfgParser::parseEntryFinish(CfgParserState &ps)
{
	if (ps.have_value) {
		parseEntryFinishStandard(ps);
	} else {
		// Template handling, expand or define template.
		if (ps.buf.size() && parseName(ps.buf) && ps.buf[0] == '@') {
			parseEntryFinishTemplate(ps.buf);
		}
		ps.buf = "";
	}
}
/**
 * Finish standard entry.
 */
void
CfgParser::parseEntryFinishStandard(CfgParserState &ps)
{
	if (parseName(ps.buf)) {
		if (ps.buf[0] == '$') {
			variableDefine(ps.buf.substr(1), ps.value);
		} else  {
			variableExpand(ps.value);

			if (ps.buf == "INCLUDE")  {
				parseSourceNew(
					ps.value,
					CfgParserSource::SOURCE_FILE);
			} else if (ps.buf == "COMMAND") {
				parseSourceNew(
					ps.value,
					CfgParserSource::SOURCE_COMMAND);
			} else {
				_section->addEntry(_source->getName(),
						   _source->getLine(),
						   ps.buf, ps.value, 0,
						   _overwrite);
			}
		}
	} else {
		USER_WARN("Dropping entry with empty name.");
	}
	ps.reset();
}

/**
 * Finish template entry, copy data into current section.
 */
void
CfgParser::parseEntryFinishTemplate(std::string &name)
{
	section_map_it it = _section_map.find(name.c_str() + 1);
	if (it == _section_map.end()) {
		P_WARN("No such template " << name);
		return;
	}

	_section->copyTreeInto(it->second);
}

void
CfgParser::parseCloseCurlyBracket(CfgParserState &ps)
{
	if (_sections.size() > 0) {
		if (ps.buf.size() && parseName(ps.buf)) {
			parseEntryFinish(ps);
			ps.reset();
		}

		if (! _opt.getEndEarlyKey().empty()
		    && pekwm::ascii_ncase_equal(_section->getName(),
						_opt.getEndEarlyKey())) {
			_is_end_early = true;
		}
		_section = _sections.back();
		_sections.pop_back();
	} else {
		USER_WARN("Extra } character found, "
			  "ignoring.");
	}
}

/**
 * Creates new Section on {
 */
void
CfgParser::parseSectionFinish(CfgParserState &ps)
{
	// Create Entry representing Section
	Entry *section = 0;
	if (ps.buf.size() == 6 && pekwm::ascii_ncase_equal(ps.buf, "DEFINE")) {
		// Look for define section, started with Define = "Name" {
		section_map_it it = _section_map.find(ps.value);
		if (it != _section_map.end()) {
			delete it->second;
			_section_map.erase(it);
		}

		section = new Entry(_source->getName(), _source->getLine(),
				    ps.buf, ps.value);
		_section_map[ps.value] = section;
	} else {
		// Expand value in Section = "$VAR" { }
		variableExpand(ps.value);

		// Create Entry for sub-section.
		section = new Entry(_source->getName(), _source->getLine(),
				    ps.buf, ps.value);

		// Add parent section, get section from parent section as it
		// can be different from the newly created if it is not
		// overwritten.
		CfgParser::Entry *parent =
			_section->addEntry(_source->getName(),
					   _source->getLine(),
					   ps.buf, ps.value, section,
					   _overwrite);
		section = parent->getSection();
	}

	// Set current Entry to newly created Section.
	_sections.push_back(_section);
	_section = section;
}

//! @brief Parses Source until end of line discarding input.
void
CfgParser::parseCommentLine(CfgParserSource *source)
{
	int c;
	while (((c = source->get_char()) != EOF) && (c != '\n'))
		;

	// Give back the newline, needed for flushing value before comment
	if (c == '\n') {
		source->unget_char(c);
	}
}

//! @brief Parses Source until */ is found.
void
CfgParser::parseCommentC(CfgParserSource *source)
{
	int c;
	while ((c = source->get_char()) != EOF) {
		if (c == '*') {
			if ((c = source->get_char()) == '/') {
				break;
			} else if (c != EOF) {
				source->unget_char(c);
			}
		}
	}

	P_LOG_IF(c == EOF, "Reached EOF before closing */ in comment.");
}

//! @brief Parses Source until next non whitespace char is found.
char
CfgParser::parseSkipBlank(CfgParserSource *source)
{
	int c;
	while (((c = source->get_char()) != EOF) && isspace(c))
		;
	if (c != EOF) {
		source->unget_char(c);
	}
	return c;
}

//! @brief Creates a CfgParserSource of type type.
CfgParserSource*
CfgParser::sourceNew(const std::string &name, CfgParserSource::Type type)
{
	CfgParserSource *source;
	switch (type) {
	case CfgParserSource::SOURCE_FILE:
		_source_name_set.insert(name);
		_source_names.push_back(name);
		source = new CfgParserSourceFile(
				*_source_name_set.find(name));
		break;
	case CfgParserSource::SOURCE_COMMAND:
		_source_name_set.insert(name);
		_source_names.push_back(name);
		source = new CfgParserSourceCommand(
				*_source_name_set.find(name), _os,
				_opt.commandPath());
		break;
	case CfgParserSource::SOURCE_STRING:
		_source_name_set.insert("string");
		_source_names.push_back("string");
		source = new CfgParserSourceString(
				*_source_name_set.find("string"),
				name);
		break;
	default:
		source = nullptr;
		break;
	}

	return source;
}

//! @brief Defines a variable in the _var_map/setenv.
void
CfgParser::variableDefine(const std::string &name, const std::string &value)
{
	_var_expander_mem->define(name, value);
}

/**
 * Expand all variables in string, recursively, updating the original string
 * until there is no more variables to expand.
 */
void
CfgParser::variableExpand(std::string& line)
{
	bool did_expand;
	do {
		did_expand = false;

		std::string var;
		std::string::size_type begin = 0, end = 0;
		for (; parseVarName(line, begin, end, var); var = "") {
			did_expand = variableExpandName(line, begin, end, var)
				|| did_expand;
			begin = end;
		}
	} while (did_expand);
}

bool
CfgParser::parseVarName(const std::string& line,
			std::string::size_type& begin,
			std::string::size_type& end,
			std::string& name)
{
	bool in_escape = false, in_var = false, in_curly = false;
	for (std::string::size_type pos = begin; pos < line.size(); pos++) {
		char c = line[pos];
		if (in_escape) {
			if (in_var) {
				name += c;
			}
			in_escape = false;
		} else if (c == '\\') {
			in_escape = true;
		} else if (in_var) {
			if (pos == (begin + 1) && c == '{') {
				in_curly = true;
			} else if (in_curly) {
				if (c == '}') {
					end = pos + 1;
					return true;
				}
				name += c;
			} else if (isalnum(c) || c == '_'
				   || (pos == (begin + 1)
				       && (c == '@' || c == '&'))) {
				name += c;
			} else {
				end = pos;
				return true;
			}
		} else if (line[pos] == '$') {
			in_var = true;
			begin = pos;
		}
	}

	// variable to end of line, not supported in {} vars at the end }
	// is required and end of line while in escape also treated as
	// error.
	if (in_var && (! in_curly && ! in_escape)) {
		end = line.size();
		return true;
	}

	// remove buffered data
	name = "";

	return false;
}

bool
CfgParser::variableExpandName(std::string &line, const size_t begin,
			      size_t &end, const std::string &var)
{
	bool did_expand = false;

	std::vector<CfgParserVarExpander*>::iterator it(_var_expanders.begin());
	for (; ! did_expand && it != _var_expanders.end(); ++it) {
		did_expand = variableExpandName(line, begin, end, var, *it);
	}

	return did_expand;
}

bool
CfgParser::variableExpandName(std::string& line, const size_t begin,
			      size_t &end, const std::string &var,
			      CfgParserVarExpander *exp)
{

	std::string value, error;
	if (exp->lookup(var, value, error)) {
		line.replace(begin, end - begin, value);
		end = begin + value.size();
		return true;
	}

	if (! error.empty()) {
		USER_WARN("failed to expand variable at "
			  << _source->getName() << ":" << _source->getLine()
			  << ":" << begin << " error: " << error);
	}
	return false;
}
