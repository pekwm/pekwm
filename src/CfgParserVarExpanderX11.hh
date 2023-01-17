//
// CfgParserVarExpanderX11.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CFG_PARSER_VAR_EXPANDER_X11_HH_
#define _PEKWM_CFG_PARSER_VAR_EXPANDER_X11_HH_

#include "CfgParserVarExpander.hh"

class CfgParserVarExpanderX11Atom : public CfgParserVarExpander {
public:
	virtual ~CfgParserVarExpanderX11Atom();
	virtual bool lookup(const std::string& name, std::string& val);
};

class CfgParserVarExpanderX11Res : public CfgParserVarExpander {
public:
	CfgParserVarExpanderX11Res(bool register_x_resource);
	virtual ~CfgParserVarExpanderX11Res();
	virtual bool lookup(const std::string& name, std::string& val);

private:
	bool _register_x_resource;
};

#endif // _PEKWM_CFG_PARSER_VAR_EXPANDER_HH_
