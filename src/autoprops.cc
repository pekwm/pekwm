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
#include <iterator>

using std::string;
using std::vector;
using std::back_insert_iterator;

AutoProps::autoproplist_item AutoProps::m_autoproplist[] = {
	{"Workspace", WORKSPACE_START},
	{"WorkspaceEnd", WORKSPACE_END},
	{"Property", PROPERTY_START},
	{"PropertyEnd", PROPERTY_END},
	{"Sticky", STICKY},
	{"Shaded", SHADED},
	{"MaximizedVertical", MAXIMIZED_VERTICAL},
	{"MaximizedHorizonal", MAXIMIZED_HORIZONTAL},
	{"Iconified", ICONIFIED},
	{"Border", BORDER},
	{"Titlebar", TITLEBAR},
	{"Position", POSITION},
	{"Size", SIZE},
	{"Layer", LAYER},
	{"AutoGroup", AUTO_GROUP},
	{"Group", GROUP},
	{"Desktop", DESKTOP},
	{"ApplyOnStart", APPLY_ON_START},
	{"ApplyOnReload", APPLY_ON_RELOAD},
	{"ApplyOnWorkspaceChange", APPLY_ON_WORKSPACE_CHANGE},
	{"ApplyOnTransient", APPLY_ON_TRANSIENT},
	{"", NO_PROPERTY}
};

AutoProps::AutoProps(Config *c) :
cfg(c)
{

}

AutoProps::~AutoProps()
{

}

void AutoProps::loadConfig(void)
{
	BaseConfig cfg(cfg->getAutoPropsFile(), "*", ";");

	if (! cfg.loadConfig()) {
		string cfg_file = DATADIR "/autoprops";
		cfg.setFile(cfg_file);
		if (! cfg.loadConfig()) {
			return;
		}
	}

	if (m_prop_list.size())
		m_prop_list.clear();

	Property property;

	string name, value;
	vector<unsigned int> workspaces;
	vector<string> values;

	AutoPropData data;

	bool parsing_property = false;

	while (cfg.getNextValue(name, value)) {
		property = getProperty(name);

		switch (property) {
		case WORKSPACE_START:
			values.clear();
			workspaces.clear();

			if (Util::splitString(value, values, ",")) {
				vector<string>::iterator it = values.begin();
				for (; it != values.end(); ++it) {
					workspaces.push_back(atoi(it->c_str()) - 1);
				}
			}

			break;
		case WORKSPACE_END:
			workspaces.clear();
			break;

		case PROPERTY_START:
			if (parsing_property) {
				m_prop_list.push_back(data);
				parsing_property = false;
			}

			if (values.size())
				values.clear();

			if ((Util::splitString(value, values, ",", 2)) == 2) {
				data.class_hint.setName(values[0]);
				data.class_hint.setClass(values[1]);
				data.class_hint.setGroup("");
				data.prop_mask = 0; // reset the data to set
				data.workspaces.clear();

				// set the workspace(s) this property should be valid for
				if (workspaces.size()) {
					copy(workspaces.begin(), workspaces.end(),
							 back_insert_iterator<vector<unsigned int> > (data.workspaces));
				}

				parsing_property = true;
			}
			break;
		case PROPERTY_END:
			if (parsing_property)
				m_prop_list.push_back(data);

			parsing_property = false;
			break;
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
			if (values.size())
				values.clear();

			if ((Util::splitString(value, values, "x", 2)) == 2) {
				data.prop_mask |= POSITION;
				data.x = atoi(values[0].c_str());
				data.y = atoi(values[1].c_str());
			}
			break;
		case SIZE:
			if (values.size())
				values.clear();

			if ((Util::splitString(value, values, "x", 2)) == 2) {
				data.prop_mask |= SIZE;
				data.width = (unsigned) atoi(values[0].c_str());
				data.height = (unsigned) atoi(values[1].c_str());
			}
			break;
		case LAYER:
			data.prop_mask |= LAYER;
			data.layer = (unsigned) atoi(value.c_str());
			break;
		case AUTO_GROUP:
			data.prop_mask |= AUTO_GROUP;
			data.auto_group = (unsigned) atoi(value.c_str());
			break;
		case GROUP:
			data.class_hint.setGroup(value);
			break;
		case DESKTOP:
			data.prop_mask |= DESKTOP;
			data.desktop = (unsigned) atoi(value.c_str()) - 1;
			break;

		default:
			// do nothing
			break;
		}
	}

	// if we forgot about PropertyEnd we might still have an property open,
	// then lets push it back to the others
	if (parsing_property) {
		m_prop_list.push_back(data);
	}
}

AutoProps::Property AutoProps::getProperty(const string &property_name)
{
	for (int i = 0; m_autoproplist[i].property != NO_PROPERTY; ++i) {
		if (m_autoproplist[i] == property_name) {
			return m_autoproplist[i].property;
		}
	}

	return NO_PROPERTY;
}

//NOTE: type and workspace defaults to -1
AutoProps::AutoPropData *AutoProps::getAutoProp(const ClassHint &class_hint,
																								int workspace,
																								int type)
{
	vector<AutoPropData>::iterator it = m_prop_list.begin();
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
				vector<unsigned int>::iterator w_it =
					find(it->workspaces.begin(), it->workspaces.end(),
							 (unsigned) workspace);

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
