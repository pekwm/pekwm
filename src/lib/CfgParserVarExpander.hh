//
// CfgParserVarExpander.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CFG_PARSER_VAR_EXPANDER_HH_
#define _PEKWM_CFG_PARSER_VAR_EXPANDER_HH_

#include <string>

enum CfgParserVarExpanderType {
	CFG_PARSER_VAR_EXPANDER_MEM,
	CFG_PARSER_VAR_EXPANDER_OS_ENV,
	CFG_PARSER_VAR_EXPANDER_X11_ATOM,
	CFG_PARSER_VAR_EXPANDER_X11_RES
};

class CfgParserVarExpander {
public:
	virtual ~CfgParserVarExpander() { }

	virtual void clear() { }
	virtual void define(const std::string& name,
			    const std::string& val) { }
	virtual bool lookup(const std::string& name, std::string& val,
			    std::string &error) = 0;
};

CfgParserVarExpander* mkCfgParserVarExpander(CfgParserVarExpanderType type,
					     bool register_x_resource);

#endif // _PEKWM_CFG_PARSER_VAR_EXPANDER_HH_
