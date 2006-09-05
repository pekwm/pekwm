//
// baseconfig.cc for pekwm
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

using std::string;
using std::vector;
using std::ifstream;

BaseConfig::BaseConfig(const string &file,
											 const char *sep1, const char *sep2) :
m_filename(file),
m_sep1(sep1), m_sep2(sep2),
m_is_loaded(false)
{
}

BaseConfig::~BaseConfig()
{
	if (m_config_list.size())
		m_config_list.clear();
}

//! @fn    bool loadConfig(void)
//! @brief Parses the config file
//! @return Returns true on success, else false
bool
BaseConfig::loadConfig(void)
{
	m_is_loaded = false;

	if (!m_filename.size())
		return false;

	ifstream config_file(m_filename.c_str());
	if (!config_file.good())
		return false;

	if (m_config_list.size()) // if we are reloading this may still be around
		m_config_list.clear();

	BaseConfigItem ins;
	const char *blanks = " \t";
	
	string line;
	string::size_type s1, t1, s2, t2;

	char buf[1024];
	while(config_file.getline(buf, 1024)) {
		line = buf;

		if (!line.length())
			continue; // empty line

		s1 = line.find_first_not_of(blanks);
		if (line[s1] == '#')
			continue; // commented line

		t2 = 0;	// TO-DO: I don't want to test the t2 stuff, speed paranoia
		do {
			s1 = line.find_first_not_of(blanks, t2 ? (t2 + 1) : 0);
			t1 = line.find_first_of(m_sep1, s1);

			s2 = line.find_first_not_of(blanks, t1 + 1);
			t2 = line.find_first_of(m_sep2, s2);

			if (s1 != string::npos && t1 != string::npos &&
					s2 != string::npos && t2 != string::npos) {

				ins.name = line.substr(s1, t1 - s1);
				ins.value = line.substr(s2, t2 - s2);

				m_config_list.push_back(ins);
			}
		} while (s1 != string::npos && t1 != string::npos &&
						 s2 != string::npos && t2 != string::npos);
	}

	config_file.close();

	m_is_loaded = true;

	return true;
}

//! @fn    bool getNextValue(string &name, string &value)
//! @brief Gets the next value from the config file.
//! @return True if any objects left in the config_list
bool
BaseConfig::getNextValue(string &name, string &value)
{
	if (m_config_list.size()) {
		m_it = m_config_list.begin();
		name = m_it->name;
		value = m_it->value;

		m_config_list.erase(m_it);

		return true;
	}

	return false;
}

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::getValue(const string &name, int &value)
{
	for (m_it = m_config_list.begin(); m_it < m_config_list.end(); ++m_it) {
		if (*m_it == name) {
			value = atoi(m_it->value.c_str());
			m_config_list.erase(m_it);
			return true;
		} 
	}
	return false;
}

//! @fn    bool getValue(const string &name, int &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::getValue(const string &name, unsigned int &value)
{
	for (m_it = m_config_list.begin(); m_it < m_config_list.end(); ++m_it) {
		if (*m_it == name) {
			value = (unsigned) atoi(m_it->value.c_str());
			m_config_list.erase(m_it);
			return true;
		}
	}
	return false;
}

//! @fn    bool getValue(const string &name, bool &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::getValue(const string &name, bool &value)
{
	for (m_it = m_config_list.begin(); m_it < m_config_list.end(); ++m_it) {
		if (*m_it == name) {
			if (! strncasecmp(m_it->value.c_str(), "true", strlen("true"))) {
				value = true;
			} else {
				value = false;
			}

			m_config_list.erase(m_it);
			return true;
		}
	}
	return false;
}

//! @fn    bool getValue(const string &name, string &value)
//! @brief Gets the value name from the config_list
//! @return True on success else false.
bool
BaseConfig::getValue(const string &name, string &value)
{
	for (m_it = m_config_list.begin(); m_it < m_config_list.end(); ++m_it) {
		if (*m_it == name) {
			value = m_it->value;
			m_config_list.erase(m_it);

			return true;
		}
	}
	return false;
}
