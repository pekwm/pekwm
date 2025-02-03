//
// CfgParserVarExpander.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

extern "C" {
#include <assert.h>
}

#include <map>

#include "CfgParserVarExpander.hh"
#include "CfgParserVarExpanderX11.hh"
#include "Debug.hh"
#include "Util.hh"

class CfgParserVarExpanderMem : public CfgParserVarExpander {
public:
	typedef std::map<std::string, std::string> var_map;
	typedef var_map::iterator var_map_it;
	typedef var_map::const_iterator var_map_cit;

	virtual ~CfgParserVarExpanderMem() { }

	virtual void clear()
	{
		_var_map.clear();
	}

	virtual void define(const std::string& name,
			    const std::string& val)
	{
		_var_map[name] = val;

		// If the variable begins with $_ it should update the
		// environment aswell.
		if ((name.size() > 1) && (name[0] == '_')) {
			Util::setEnv(name.c_str() + 1, val);
		}
	}

	virtual bool lookup(const std::string& name, std::string& val,
			    std::string &error)
	{
		var_map_it it = _var_map.find(name);
		if (it != _var_map.end()) {
			val = it->second;
		} else  {
			error = "\"" + name + "\" not set";
		}
		return it != _var_map.end();
	}

private:
	var_map _var_map;
};

class CfgParserVarExpanderOsEnv : public CfgParserVarExpander {
public:
	virtual ~CfgParserVarExpanderOsEnv() { }

	virtual bool lookup(const std::string& name, std::string& val,
			    std::string &error)
	{
		if (name.size() < 2 || name[0] != '_') {
			return false;
		}

		char *c_val = getenv(name.c_str() + 1);
		if (c_val) {
			val = c_val;
		} else {
			error = "\"" + name + "\" not set in environment";
		}
		return c_val != nullptr;
	}
};

/**
 * Construct a CfgParserVarExpander of the given type.
 */
CfgParserVarExpander*
mkCfgParserVarExpander(CfgParserVarExpanderType type,
		       bool register_x_resource)
{
	switch (type) {
	case CFG_PARSER_VAR_EXPANDER_MEM:
		return new CfgParserVarExpanderMem();
	case CFG_PARSER_VAR_EXPANDER_OS_ENV:
		return new CfgParserVarExpanderOsEnv();
#ifndef PEKWM_DISABLE_X11_CFG_PARSER_VAR_EXPANDER
	case CFG_PARSER_VAR_EXPANDER_X11_ATOM:
		return new CfgParserVarExpanderX11Atom();
	case CFG_PARSER_VAR_EXPANDER_X11_RES:
		return new CfgParserVarExpanderX11Res(register_x_resource);
#endif // PEKWM_DISABLE_X11_CFG_PARSER_VAR_EXPANDER
	default:
		return nullptr;
	}
}
