//
// BaseConfig.hh
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//                         Alaa Abd El Fatah <alaa@linux-egypt.org>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _BASECONFIG_HH_
#define _BASECONFIG_HH_

#include <string>
#include <algorithm>
#include <list>
#include <cstring>
#include <fstream>

class BaseConfig
{
public:
	struct CfgItem {
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
		CfgSection(const std::string &n="") : _name(n) { };
		void load(std::istream& file);
		bool load(std::string& filename);
		inline const std::string& getName(void) { return _name; }

		inline CfgSection* getSection(const std::string& name) {
			std::list<CfgSection>::iterator it =
				find(_sect_list.begin(), _sect_list.end(), name);
			if (it != _sect_list.end())
				return &*it;
			return NULL;
		}

		inline CfgSection* getNextSection(void) {
			if (_sect_it != _sect_list.end())
				return &*_sect_it++;
			return NULL;
		}

		bool getValue(const std::string &name, int &value);
		bool getValue(const std::string &name, unsigned int &value);
		bool getValue(const std::string &name, bool &value);
		bool getValue(const std::string &name, std::string &value);
		bool getNextValue(std::string &name, std::string &value);

		// Well, it seems as if gcc-2.95.X uses != in find and
		// gcc-3.x uses == . So I have to have both
		inline bool operator == (const std::string &s) {
			return !(strcasecmp(_name.c_str(), s.c_str()));
		}
		inline bool operator != (const std::string &s) {
			return (strcasecmp(_name.c_str(), s.c_str()));
		}

	private:
		std::string _name;
		std::list<CfgItem> _values;
		std::list<CfgItem>::iterator _it;
		std::list<CfgSection> _sect_list;
		std::list<CfgSection>::iterator _sect_it;
	};

	BaseConfig() { }
	~BaseConfig() { }

	bool load(const std::string &file);
	bool loadCommand(const std::string &command);

	inline CfgSection* getSection(const std::string& name) {
	  return _cfg.getSection(name);
	}
	inline CfgSection* getNextSection(void) {
	  return _cfg.getNextSection();
	}

private:
  CfgSection _cfg;
};

#endif // _BASECONFIG_HH_
