//
// baseconfig.hh
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "baseconfig.hh"

#include <cstdio>
#include <functional>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::string;
using std::list;
using std::mem_fun;

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, int &value)
{
	m_it = find(m_values.begin(), m_values.end(), name);
	if (m_it != m_values.end()) {
		value = atoi(m_it->value.c_str());
		m_values.erase(m_it);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, unsigned int &value)
{
	m_it = find(m_values.begin(), m_values.end(), name);
	if (m_it != m_values.end()) {
		value = (unsigned) atoi(m_it->value.c_str());
		m_values.erase(m_it);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, bool &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, bool &value)
{
	m_it = find(m_values.begin(), m_values.end(), name);
	if (m_it != m_values.end()) {
		if (! strncasecmp(m_it->value.c_str(), "true", strlen("true")))
			value = true;
		else
			value = false;
		m_values.erase(m_it);
		return true;
	}
	return false;
}

//! @fn    bool getValue(const string &name, string &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::CfgSection::getValue(const string &name, string &value)
{
	m_it = find(m_values.begin(), m_values.end(), name);
	if (m_it != m_values.end()) {
		value = m_it->value;
		m_values.erase(m_it);
		return true;
	}
	return false;
}

//! @fn    bool getNextValue(string &name, string &value)
//! @brief Gets the next value from the config file.
//! @return True if any objects left in the config_list
bool
BaseConfig::CfgSection::getNextValue(string &name, string &value)
{
	if (m_values.size()) {
		name = m_values.front().name;
		value = m_values.front().value;
		m_values.pop_front();
		return true;
	}
	return false;
}

BaseConfig::BaseConfig()
{

}

BaseConfig::~BaseConfig()
{
	if (m_cfg_list.size())
		m_cfg_list.clear();
}

//! @fn    bool load(const string &filename)
//! @brief Parses the file, unloads allready parsed info.
//! @param filename Name of file to parse.
bool
BaseConfig::load(const string &filename)
{
	if (!filename.size())
		return false;

	FILE *file = fopen(filename.c_str(), "r");
	if (file) {
		bool success = load(file);
		fclose(file);

		return success;
	}
	return false;
}

//! @fn    bool load(FILE *file)
//! @brief Parses the FILE object, unloads allready parsed info.
//! @param file FILE* to parse.
bool
BaseConfig::load(FILE *file)
{
	if (!file)
		return false;

	if (m_cfg_list.size()) // make sure nothing old is here
		m_cfg_list.clear();

	CfgItem ci; // tmp item
	list<CfgSection*> curr_section; // to keep track of current section

	string line; // string used for parsing
	string name, value; // used before pushing into the list

	string::size_type pos, start;
	string::size_type pos_1, pos_2, pos_3;

	char buf[1024];
	while (!feof(file) && !ferror(file) && fgets(buf, 1024, file)) {
		if (!strlen(buf))
			continue; // empty line
		line = buf;

		start = line.find_first_not_of(" \t");

		if ((start == string::npos) || line[start] == '#')
			continue; // commented line

		// okie, lets see if this is a start of a property
		pos = line.find_first_of('{', start);
		if (pos != string::npos) {
			pos = line.find_first_of(" \t{", start); // fix leading blanks
			name = line.substr(start, pos - start);

			CfgSection section(name);

			if (curr_section.size()) {
				curr_section.push_back(curr_section.back()->addCfgSection(section));
			} else {
				m_cfg_list.push_back(section);
				curr_section.push_back(&m_cfg_list.back());
			}

			continue; // continue to next line
		} else if (curr_section.size()) {
			// or if it's an end
			pos = line.find_first_of('}');
			if (pos != string::npos) {
				curr_section.back()->resetSection();
				curr_section.pop_back();
				continue; // continue to next line
			}

			pos_1 = line.find_first_of('=', start);
			if (pos_1 != string::npos) {
				pos_2 = line.find_first_of('"', pos_1 + 1);
				pos_3 = line.find_first_of('"', pos_2 + 1);
				if ((pos_2 != string::npos) && (pos_3 != string::npos)) {
					pos_1 = line.find_first_of(" \t=", start); // fix the blanks
					++pos_2; // fix leading "

					ci.name = line.substr(start, pos_1 - start);
					ci.value = line.substr(pos_2, pos_3 - pos_2);

					curr_section.back()->addCfgItem(ci);
				}
			}
		}
	}

	if (curr_section.size()) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Resetting " << curr_section.size() << " section(s)." << endl;
#endif // DEBUG
		for_each(curr_section.begin(), curr_section.end(),
						 mem_fun(&BaseConfig::CfgSection::resetSection));
	}

	// This iterator is used in getNextSection so that you can go through
	// the sections withouth knowing their names.
	m_sect_it = m_cfg_list.begin();

	return true;
}
