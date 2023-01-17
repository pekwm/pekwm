//
// CfgParserVarExpanderX11.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParserVarExpanderX11.hh"
#include "X11.hh"

CfgParserVarExpanderX11Atom::~CfgParserVarExpanderX11Atom()
{
}

bool
CfgParserVarExpanderX11Atom::lookup(const std::string& name, std::string& val)
{
	if (name.size() < 2 || name[0] != '@') {
		return false;
	}

	std::string atom_name(name.substr(1));

	Atom id;
	AtomName aname = X11::getAtomName(atom_name);
	if (aname == MAX_NR_ATOMS) {
		id = X11::getAtomId(atom_name);
	} else {
		id = X11::getAtom(aname);
	}

	return X11::getStringId(X11::getRoot(), id, val);
}

CfgParserVarExpanderX11Res::CfgParserVarExpanderX11Res(
		bool register_x_resource)
	: _register_x_resource(register_x_resource)
{
}

CfgParserVarExpanderX11Res::~CfgParserVarExpanderX11Res()
{
}

bool
CfgParserVarExpanderX11Res::lookup(const std::string& name, std::string& val)
{
	if (name.size() < 2 || name[0] != '&') {
		return false;
	}
	std::string res_name = name.substr(1);
	bool found = X11::getXrmString(res_name, val);
	if (_register_x_resource) {
		X11::registerRefResource(res_name, val);
	}
	return found;
}
