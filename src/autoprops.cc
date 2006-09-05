//
// autoprops.cc for pekwm
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

#include "autoprops.hh"
#include "baseconfig.hh"
#include "config.hh"
#include "util.hh"

#include <algorithm>
#include <vector>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;
using std::vector;

AutoProps::autoproplist_item AutoProps::m_autoproplist[] = {
	{"WORKSPACE", WORKSPACE},
	{"PROPERTY", PROPERTY},
	{"STICKY", STICKY},
	{"SHADED", SHADED},
	{"MAXIMIZEDVERTICAL", MAXIMIZED_VERTICAL},
	{"MAXIMIZEDHORIZONTAL", MAXIMIZED_HORIZONTAL},
	{"ICONIFIED", ICONIFIED},
	{"BORDER", BORDER},
	{"TITLEBAR", TITLEBAR},
	{"POSITION", POSITION},
	{"SIZE", SIZE},
	{"LAYER", LAYER},
	{"AUTOGROUP", AUTO_GROUP},
	{"GROUP", GROUP},
	{"APPLYONSTART", APPLY_ON_START},
	{"APPLYONRELOAD", APPLY_ON_RELOAD},
	{"APPLYONWORKSPACECHANGE", APPLY_ON_WORKSPACE_CHANGE},
	{"APPLYONTRANSIENT", APPLY_ON_TRANSIENT},
	{"", NO_PROPERTY}
};

AutoProps::AutoProps(Config *c) :
cfg(c)
{
}

AutoProps::~AutoProps()
{
}

//! @fn    void loadConfig(void)
//! @brief Loads the autoprop config file.
void
AutoProps::loadConfig(void)
{
	if (m_prop_list.size())
		m_prop_list.clear();

	BaseConfig a_cfg;
	if (!a_cfg.load(cfg->getAutoPropsFile())) {
		string cfg_file = DATADIR "/autoprops";
		if (!a_cfg.load(cfg_file)) {
			return;
		}
	}

	string value;
	vector<string> tokens;
	vector<string>::iterator it;
	list<unsigned int> workspaces;

	BaseConfig::CfgSection *base, *sub;
	while ((base = a_cfg.getNextSection())) {
		// Is this a workspace section?
		if (*base == "WORKSPACE") {
			if (!base->getValue("WORKSPACE", value))
				continue; // we need workspace numbers

			tokens.clear();
			if (Util::splitString(value, tokens, " \t")) {
				workspaces.clear();
				for (it = tokens.begin(); it != tokens.end(); ++it) {
					workspaces.push_back(atoi(it->c_str()) - 1);
				}
			} else
				continue; // we still need workspacw numbers

			// get all properties on for these workspaces
			while ((sub = base->getNextSection()))
				parseProperty(sub, &workspaces);

		} else {
			parseProperty(base, NULL);
		}
	}
}

//! @fn    void parseProperty(BaseConfig::CfgSection *cs, list<unsigned int> *ws)
//! @brief
void
AutoProps::parseProperty(BaseConfig::CfgSection *cs, list<unsigned int> *ws)
{
	if (!cs)
		return;

	AutoPropData data;
	string name, value;
	vector<string> tokens;

	if (!cs->getValue("CLASS", value))
		return; // we need a class

	if ((Util::splitString(value, tokens, ",", 2)) == 2) {
		data.class_hint.setName(tokens[0]);
		data.class_hint.setClass(tokens[1]);
	} else
		return; // we still need a class

	if (ws) // Workspace?
		data.workspaces.assign(ws->begin(), ws->end());

	Property property;
	while (cs->getNextValue(name, value)) {
		property = getProperty(name);

		switch (property) {
		case STICKY:
			data.prop_mask |= STICKY;
			data.sticky = Util::isTrue(value);
			break;
		case SHADED:
			data.prop_mask |= SHADED;
			data.shaded = Util::isTrue(value);
			break;
		case MAXIMIZED_VERTICAL:
			data.prop_mask |= MAXIMIZED_VERTICAL;
			data.maximized_vertical = Util::isTrue(value);
			break;
		case MAXIMIZED_HORIZONTAL:
			data.prop_mask |= MAXIMIZED_HORIZONTAL;
			data.maximized_horizontal = Util::isTrue(value);
			break;
		case ICONIFIED:
			data.prop_mask |= ICONIFIED;
			data.iconified = Util::isTrue(value);
			break;
		case BORDER:
			data.prop_mask |= BORDER;
			data.border = Util::isTrue(value);
			break;
		case TITLEBAR:
			data.prop_mask |= TITLEBAR;
			data.titlebar = Util::isTrue(value);
			break;
		case APPLY_ON_TRANSIENT:
			data.prop_mask |= APPLY_ON_TRANSIENT;
			data.apply_on_transient = Util::isTrue(value);
			break;
		case APPLY_ON_START:
			data.prop_mask |= APPLY_ON_START;
			data.apply_on_start = Util::isTrue(value);
			break;
		case APPLY_ON_RELOAD:
			data.prop_mask |= APPLY_ON_RELOAD;
			data.apply_on_reload = Util::isTrue(value);
			break;
		case APPLY_ON_WORKSPACE_CHANGE:
			data.prop_mask |= APPLY_ON_WORKSPACE_CHANGE;
			data.apply_on_workspace_change = Util::isTrue(value);
			break;
		case POSITION:
			if (tokens.size())
				tokens.clear();

			if ((Util::splitString(value, tokens, "x", 2)) == 2) {
				data.prop_mask |= POSITION;
				data.x = atoi(tokens[0].c_str());
				data.y = atoi(tokens[1].c_str());
			}
			break;
		case SIZE:
			if (tokens.size())
				tokens.clear();

			if ((Util::splitString(value, tokens, "x", 2)) == 2) {
				data.prop_mask |= SIZE;
				data.width = unsigned(atoi(tokens[0].c_str()));
				data.height = unsigned(atoi(tokens[1].c_str()));
			}
			break;
		case LAYER:
			data.prop_mask |= LAYER;
			data.layer = unsigned(atoi(value.c_str()));
			break;
		case AUTO_GROUP:
			data.prop_mask |= AUTO_GROUP;
			data.auto_group = unsigned(atoi(value.c_str()));
			break;
		case GROUP:
			data.class_hint.setGroup(value);
			break;
		case WORKSPACE:
			data.prop_mask |= WORKSPACE;
			data.workspace = unsigned(atoi(value.c_str()) - 1);
			break;

		default:
			// do nothing
			break;
		}
	}

	m_prop_list.push_back(data);
}

//! @fn    AutoProps::Property getProperty(const string &property_name)
//! @brief Tries to match a property against the property_name.
AutoProps::Property
AutoProps::getProperty(const string &property_name)
{
	if (!property_name.size())
		return NO_PROPERTY;

	for (unsigned int i = 0; m_autoproplist[i].property != NO_PROPERTY; ++i) {
		if (m_autoproplist[i] == property_name) {
			return m_autoproplist[i].property;
		}
	}

	return NO_PROPERTY;
}

//NOTE: type and workspace defaults to -1
//! @fn    AutoProps::AutoPropsData* getAutoProp(const ClassHint &class_hint, int workspace, int type)
//! @brief Tries matching a ClassHint against a autoprop.
//! @param workspace On what workspace is the client, defaults to -1
//! @param type When do we reqeuest this, defaults to -1.
AutoProps::AutoPropData*
AutoProps::getAutoProp(const ClassHint &class_hint, int workspace, int type)
{
	list<AutoPropData>::iterator it = m_prop_list.begin();
	for (; it != m_prop_list.end(); ++it) {
		// see if we set type of event
		if (type != -1) {
			switch(type) {
			case APPLY_ON_START:
				if (!it->applyOnStart() || !it->apply_on_start)
					continue;
				break;
			case APPLY_ON_RELOAD:
				if (!it->applyOnReload() || !it->apply_on_reload)
					continue;
				break;
			case APPLY_ON_WORKSPACE_CHANGE:
				if (!it->applyOnWorkspaceChange() || !it->apply_on_workspace_change)
					continue;
				break;
			default:
				break;
			}
		}

		// I can't use the overloaded operater == as it also includes the group
		// wich I don't want to do here
		ClassHint *c = &it->class_hint; // convinience
		if (((class_hint.getName() == "*") || (c->getName() == "*") ||
				 (class_hint.getName() == c->getName())) &&
				 ((class_hint.getClass() == "*") || (c->getClass() == "*") ||
					(class_hint.getClass() == c->getClass()))) {

			// see if this property applies for the workspace we're on
			if (it->workspaces.size()) {
				list<unsigned int>::iterator w_it =
					find(it->workspaces.begin(), it->workspaces.end(),
							 unsigned(workspace));

				if (w_it != it->workspaces.end()) {
					return &*it;
				}
			} else {
				return &*it;
			}
		}
	}

	return NULL;
}
