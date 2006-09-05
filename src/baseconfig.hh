//
// baseconfig.hh
// Copyright (C) 2002 Claes Nasten
// pekdon@gmx.net 
// http://pekdon.babblica.net/
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

#ifndef _BASECONFIG_HH_
#define _BASECONFIG_HH_

#include <string>
#include <vector>
#include <fstream>

class BaseConfig
{
	struct BaseConfigItem {
		std::string name;
		std::string value;
		inline bool operator == (std::string s) {
			return (strcasecmp(name.c_str(), s.c_str()) ? false : true);
		}
	};

public:
	BaseConfig(const std::string &file, const char *sep1, const char *sep2);
	~BaseConfig();

	bool loadConfig(void);
	inline void setFile(const std::string &f) { m_filename = f; }
	inline bool isLoaded(void) const { return m_is_loaded; }

	bool getValue(const std::string &name, int &value);
	bool getValue(const std::string &name, unsigned int &value);
	bool getValue(const std::string &name, bool &value);
	bool getValue(const std::string &name, std::string &value);

	bool getNextValue(std::string &name, std::string &value);


private:
	std::string m_filename;
	const char *m_sep1, *m_sep2;

	std::vector<BaseConfigItem> m_config_list;
	std::vector<BaseConfigItem>::iterator m_it;

	bool m_is_loaded;
};

#endif // _BASECONFIG_HH_
