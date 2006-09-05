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

#ifndef _BASECONFIG_HH_
#define _BASECONFIG_HH_

#include <string>
#include <algorithm>
#include <list>
#include <cstdio>
#include <cstring>

class BaseConfig
{
public:
	struct CfgItem {
	public:
		std::string name;
		std::string value;
		// Well, it seems as if gcc-2.95.X uses != in find and
		// gcc-3.x uses == . So I have to have both
		inline bool operator == (const std::string &s) {
			return (strcasecmp(name.c_str(), s.c_str()) ? false : true);
		}
		inline bool operator != (const std::string &s) {
			return (strcasecmp(name.c_str(), s.c_str()));
		}
	};

	class CfgSection {
	public:
		CfgSection(const std::string &n) : m_name(n) { };

		inline const std::string &getName(void) { return m_name; }

		inline CfgSection* getSection(const std::string &name) {
			std::list<CfgSection>::iterator it =
				find(m_sect_list.begin(), m_sect_list.end(), name);
			if (it != m_sect_list.end())
				return &*it;
			return NULL;
		}

		inline CfgSection* getNextSection(void) {
			if (m_sect_it != m_sect_list.end())
				return &*m_sect_it++;
			return NULL;
		}

		bool getValue(const std::string &name, int &value);
		bool getValue(const std::string &name, unsigned int &value);
		bool getValue(const std::string &name, bool &value);
		bool getValue(const std::string &name, std::string &value);
		bool getNextValue(std::string &name, std::string &value);

		inline void addCfgItem(const CfgItem &ci) { m_values.push_back(ci); }
		inline CfgSection* addCfgSection(const CfgSection &cs) {
			m_sect_list.push_back(cs);
			return &m_sect_list.back();
		}
		void resetSection(void) { m_sect_it = m_sect_list.begin(); }

		// Well, it seems as if gcc-2.95.X uses != in find and
		// gcc-3.x uses == . So I have to have both
		inline bool operator == (const std::string &s) {
			return (strcasecmp(m_name.c_str(), s.c_str()) ? false : true);
		}
		inline bool operator != (const std::string &s) {
			return (strcasecmp(m_name.c_str(), s.c_str()));
		}

	private:
		std::string m_name;
		std::list<CfgItem> m_values;
		std::list<CfgItem>::iterator m_it;
		std::list<CfgSection> m_sect_list;
		std::list<CfgSection>::iterator m_sect_it;
	};

	BaseConfig();
	~BaseConfig();

	bool load(const std::string &file);
	bool load(FILE *file);

	inline CfgSection* getSection(const std::string &name) {
		m_it = find(m_cfg_list.begin(), m_cfg_list.end(), name);
		if (m_it != m_cfg_list.end())
			return &*m_it;
		return NULL;
	}

	inline CfgSection* getNextSection(void) {
		if (m_sect_it != m_cfg_list.end())
			return &*m_sect_it++;
		return NULL;
	}

private:
	void unload(void);

private:
	std::list<CfgSection> m_cfg_list;
	std::list<CfgSection>::iterator m_it, m_sect_it;
};

#endif // _BASEMENU_HH_
