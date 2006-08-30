//
// AutoProperties.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "AutoProperties.hh"

#include "Config.hh"
#include "Util.hh"

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

// global properties
AutoProperties::propertylist_item AutoProperties::_propertylist[] = {
	{"WORKSPACE", AP_WORKSPACE},
	{"PROPERTY", AP_PROPERTY},
	{"STICKY", AP_STICKY},
	{"SHADED", AP_SHADED},
	{"MAXIMIZEDVERTICAL", AP_MAXIMIZED_VERTICAL},
	{"MAXIMIZEDHORIZONTAL", AP_MAXIMIZED_HORIZONTAL},
	{"ICONIFIED", AP_ICONIFIED},
	{"BORDER", AP_BORDER},
	{"TITLEBAR", AP_TITLEBAR},
	{"GEOMETRY", AP_GEOMETRY},
	{"LAYER", AP_LAYER},
	{"SKIP", AP_SKIP},
	{"", AP_NO_PROPERTY}
};

// group properties
AutoProperties::propertylist_item AutoProperties::_grouppropertylist[] = {
	{"SIZE", AP_GROUP_SIZE},
	{"NAME", AP_GROUP_NAME},
	{"BEHIND", AP_GROUP_BEHIND},
	{"FOCUSEDFIRST", AP_GROUP_FOCUSED_FIRST},
	{"GLOBAL", AP_GROUP_GLOBAL},
	{"", AP_NO_PROPERTY}
};

//! @fn    AutoProperties()
//! @brief Constructor for AutoProperties class
AutoProperties::AutoProperties(Config *c) :
_cfg(c),
_apply_on_start(true)
{
}

//! @fn    ~AutoProperties()
//! @brief Destructor for AutoProperties class
AutoProperties::~AutoProperties()
{
	unloadConfig();
}

//! @fn    void loadConfig(void)
//! @brief Loads the autoprop config file.
void
AutoProperties::loadConfig(void)
{
	// dealloc memory
	unloadConfig();

	BaseConfig a_cfg;
	if (!a_cfg.load(_cfg->getAutoPropsFile())) {
		string cfg_file = SYSCONFDIR "/autoproperties";
		if (!a_cfg.load(cfg_file)) {
			return;
		}
	}

	// reset values
	_apply_on_start = true;

	string value;
	vector<string> tokens;
	vector<string>::iterator it;
	list<unsigned int> workspaces;

	BaseConfig::CfgSection *base, *sub;
	while ((base = a_cfg.getNextSection())) {
		if (*base == "TITLERULES") { // TitleRule section
			parseTitleProperty(base);
		} else if (*base == "WORKSPACE") { // Workspace section
			if (!base->getValue("WORKSPACE", value))
				continue; // we need workspace numbers

			tokens.clear();
			if (Util::splitString(value, tokens, " \t")) {
				workspaces.clear();
				for (it = tokens.begin(); it != tokens.end(); ++it) {
					workspaces.push_back(atoi(it->c_str()) - 1);
				}

				// get all properties on for these workspaces
				while ((sub = base->getNextSection()))
					parseAutoProperty(sub, &workspaces);
			}
		} else {
			parseAutoProperty(base, NULL);
		}
	}
}

//! @fn    void unloadConfig(void)
//! @brief Frees allocated memory
void
AutoProperties::unloadConfig(void)
{
	// remove autoprops
	if (_prop_list.size()) {
		list<Property*>::iterator it = _prop_list.begin();
		for (; it != _prop_list.end(); ++it) {
			delete *it;
		}
		_prop_list.clear();		
	}

	// remove title properties
	if (_title_prop_list.size()) {
		list<Property*>::iterator it =
			_title_prop_list.begin();		
		for (; it != _title_prop_list.end(); ++it) {
			delete *it;
		}
		_title_prop_list.clear();
	}
}

//! @fn    Property* findProperty(const ClassHint* class_hint, list<Property*>* prop_list, int ws, unsigned int type)
//! @brief Finds a property from the prop_list
Property*
AutoProperties::findProperty(const ClassHint* class_hint,
														 list<Property*>* prop_list,
														 int ws, unsigned int type)
{
	// Allready remove apply on start
	if (!_apply_on_start && (type == APPLY_ON_START))
		return NULL;

	list<Property*>::iterator it = prop_list->begin();
	list<unsigned int>::iterator w_it;

	// start searching for a suitable property
	for (bool ok = false; it != prop_list->end(); ++it, ok = false) {
		// see if the type matches, if we have one
		if (type && !(*it)->isApplyOn(type))
				continue;

		// match the class
		if ((*it)->getHintName().isRegOk() && (*it)->getHintClass().isRegOk()) {
			if (((*it)->getHintName() == class_hint->h_name) &&
					((*it)->getHintClass() == class_hint->h_class)) {

				if ((*it)->getTitle().isRegOk()) // got a title match it too
					ok = ((*it)->getTitle() == class_hint->title);
				else
					ok = true;

			}
		} else if ((*it)->getTitle().isRegOk()) {
			ok = ((*it)->getTitle() == class_hint->title);
		}

		// it matched against either title or class or both
		if (ok) {
			if ((*it)->getWsList().size()) {
				w_it = find((*it)->getWsList().begin(), (*it)->getWsList().end(),
										unsigned(ws));
				if (w_it != (*it)->getWsList().end())
					return *it;
			} else {
				return *it;
			}
		}
	}

	return NULL;
}

//! @fn    bool parseProperty(BaseConfig::CfgSection* cs, Property* property);
//! @brief Parses a cs and sets up a basic property
bool
AutoProperties::parseProperty(BaseConfig::CfgSection* cs, Property* property)
{
	bool ok = false;
	string value;
	vector<string> tokens;
	vector<string>::iterator it;

	// first see if we have a title4
	if (cs->getValue("TITLE", value)) {
		property->getTitle() = value;
		ok = property->getTitle().isRegOk();
	}
	// then search for a class
	if (cs->getValue("NAME", value) &&
			((Util::splitString(value, tokens, ",", 2)) == 2)) {
		property->getHintName() = tokens[0];
		property->getHintClass() = tokens[1];
		if (property->getHintName().isRegOk() &&
				property->getHintClass().isRegOk())
			ok = true;
	}

	if (ok) {
		// parse apply on mask
		if (cs->getValue("APPLYON", value)) {
			tokens.clear();
			if ((Util::splitString(value, tokens, " \t", 5))) {
				for (it = tokens.begin(); it != tokens.end(); ++it)
					property->applyAdd((unsigned int) _cfg->getApplyOn(*it));
			}
		}
	}

	return ok;
}

//! @fn    void parseProperty(BaseConfig::CfgSection* cs, list<unsigned int>* ws)
//! @brief
void
AutoProperties::parseAutoProperty(BaseConfig::CfgSection* cs,
																	list<unsigned int>* ws)
{
	AutoProperty* property = new AutoProperty();

	if (!parseProperty(cs, property)) {
		delete property;

		return; // not a valid property
	}

	// copy workspaces, if any
	if (ws)
		property->getWsList().assign(ws->begin(), ws->end());

	// see if we have a group section
	BaseConfig::CfgSection *sub = cs->getSection("GROUP");
	if (sub)
		parseAutoGroup(sub, property);

	// start parsing of values
	string name, value;
	vector<string> tokens;
	vector<string>::iterator it;

	PropertyType property_type;
	while (cs->getNextValue(name, value)) {
		property_type = getProperty(name);

		switch (property_type) {
		case AP_STICKY:
			property->maskAdd(AP_STICKY);
			property->sticky = Util::isTrue(value);
			break;
		case AP_SHADED:
			property->maskAdd(AP_SHADED);
			property->shaded = Util::isTrue(value);
			break;
		case AP_MAXIMIZED_VERTICAL:
			property->maskAdd(AP_MAXIMIZED_VERTICAL);
			property->maximized_vertical = Util::isTrue(value);
			break;
		case AP_MAXIMIZED_HORIZONTAL:
			property->maskAdd(AP_MAXIMIZED_HORIZONTAL);
			property->maximized_horizontal = Util::isTrue(value);
			break;
		case AP_ICONIFIED:
			property->maskAdd(AP_ICONIFIED);
			property->iconified = Util::isTrue(value);
			break;
		case AP_BORDER:
			property->maskAdd(AP_BORDER);
			property->border = Util::isTrue(value);
			break;
		case AP_TITLEBAR:
			property->maskAdd(AP_TITLEBAR);
			property->titlebar = Util::isTrue(value);
			break;
		case AP_GEOMETRY:
			property->maskAdd(AP_GEOMETRY);
			property->gm_mask =
				XParseGeometry((char*) value.c_str(), &property->gm.x, &property->gm.y,
												&property->gm.width, &property->gm.height);
			break;
		case AP_LAYER:
			property->maskAdd(AP_LAYER);
			property->layer = _cfg->getLayer(value);
			break;
		case AP_WORKSPACE:
			property->maskAdd(AP_WORKSPACE);
			property->workspace = unsigned(atoi(value.c_str()) - 1);
			break;
		case AP_SKIP:
			property->maskAdd(AP_SKIP);
			tokens.clear();
			if ((Util::splitString(value, tokens, " \t"))) {
				for (it = tokens.begin(); it != tokens.end(); ++it)
					property->skip |= _cfg->getSkip(*it);
			}
			break;

		default:
			// do nothing
			break;
		}
	}

	_prop_list.push_back(property);
}

//! @fn    void parseAutoGroup(BaseConfig::CfgSection* cs, AutoProperty* property)
//! @brief Parses a Group section of the AutoProps
void
AutoProperties::parseAutoGroup(BaseConfig::CfgSection* cs,
															 AutoProperty* property)
{
	string name, value;

	PropertyType property_type;
	while (cs->getNextValue(name, value)) {
		property_type = getGroupProperty(name);

		switch (property_type) {
		case AP_GROUP_SIZE:
			property->group_size = unsigned(atoi(value.c_str()));
			break;
		case AP_GROUP_NAME:
			property->group_name = value;
			break;
		case AP_GROUP_BEHIND:
			property->group_behind = Util::isTrue(value);
			break;
		case AP_GROUP_FOCUSED_FIRST:
			property->group_focused_first = Util::isTrue(value);
			break;
		case AP_GROUP_GLOBAL:
			property->group_global = Util::isTrue(value);
			break;
		default:
			// do nothing
			break;
		};
	}
}

//! @fn    void parseTitleProperty(BaseConfig::CfgSection* cs)
//! @brief Parses a title property section
void
AutoProperties::parseTitleProperty(BaseConfig::CfgSection* cs)
{
	TitleProperty *property;

	bool ok;
	string value;

	BaseConfig::CfgSection *sub;
	while ((sub = cs->getNextSection())) {
		property = new TitleProperty();

		ok = false;

		if (parseProperty(sub, property)) {
			if (sub->getValue("RULE", value)) {
				property->getTitleRule().parse(value);
				if (property->getTitleRule().isOk()) {
					_title_prop_list.push_back(property);
					ok = true;
				}
			}
		}

		if (!ok)
			delete property;
	}
}

//! @fn    AutoProperty* findAutoProperty(const ClassHint* class_hint, int ws, unsigned int type)
//! @brief Searches the _prop_list for a property
AutoProperty*
AutoProperties::findAutoProperty(const ClassHint* class_hint,
																 int ws, unsigned int type)
{
	AutoProperty *property =
		(AutoProperty*) findProperty(class_hint, &_prop_list, ws, type);
	if (property)
		return (AutoProperty*) property;
	return NULL;
}

//! @fn    TitleProperty* findTitleProperty(const ClassHint* class_hint, unsigned int type)
//! @brief Searches the _title_prop_list for a property
TitleProperty*
AutoProperties::findTitleProperty(const ClassHint* class_hint,
																	unsigned int type)
{
	TitleProperty *property =
		(TitleProperty*) findProperty(class_hint, &_title_prop_list, -1, type);
	if (property)
		return (TitleProperty*) property;
	return NULL;
}

//! @fn    void removeApplyOnStart(void)
//! @brief Removes all ApplyOnStart actions as they consume memory
void
AutoProperties::removeApplyOnStart(void)
{
	list<Property*>::iterator it = _prop_list.begin();
	for (; it != _prop_list.end(); ++it) {
		if ((*it)->isApplyOn(APPLY_ON_START)) {
			(*it)->applyRemove(APPLY_ON_START);
			if (!(*it)->getApplyOn()) {
				delete *it;
				it = _prop_list.erase(it);
				--it; // compensate for the ++ in the loop
			}
		}
	}

	_apply_on_start = false;
}

//! @fn    PropertyType getProperty(const string& property_name) const
//! @brief Tries to match a property against the property_name.
PropertyType
AutoProperties::getProperty(const string& property_name) const
{
	if (!property_name.size())
		return AP_NO_PROPERTY;
	for (unsigned int i = 0; _propertylist[i].property != AP_NO_PROPERTY; ++i) {
		if (_propertylist[i] == property_name)
			return _propertylist[i].property;
	}
	return AP_NO_PROPERTY;
}

//! @fn    PropertyType getGroupProperty(const string& property_name) const
//! @brief Tries to match a property against the property_name.
PropertyType
AutoProperties::getGroupProperty(const string& property_name) const
{
	if (!property_name.size())
		return AP_NO_PROPERTY;
	for (unsigned int i = 0; _grouppropertylist[i].property != AP_NO_PROPERTY; ++i) {
		if (_grouppropertylist[i] == property_name)
			return _grouppropertylist[i].property;
	}
	return AP_NO_PROPERTY;
}
