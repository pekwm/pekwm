//
// AutoProperties.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _AUTOPROPERTIES_HH_
#define _AUTOPROPERTIES_HH_

#include "pekwm.hh"
#include "BaseConfig.hh"
#include "RegexString.hh"

#include <string>
#include <list>

class Config;
class RegexString;

enum PropertyType {
	AP_STICKY = (1<<1),
	AP_SHADED = (1<<2),
	AP_MAXIMIZED_VERTICAL = (1<<3),
	AP_MAXIMIZED_HORIZONTAL = (1<<4),
	AP_ICONIFIED = (1<<5),
	AP_BORDER = (1<<6),
	AP_TITLEBAR = (1<<7),
	AP_GEOMETRY = (1<<8),
	AP_LAYER = (1<<9),
	AP_WORKSPACE = (1<<10),
	AP_SKIP = (1<<11),

	AP_GROUP_SIZE,
	AP_GROUP_NAME,
	AP_GROUP_BEHIND,
	AP_GROUP_FOCUSED_FIRST,
	AP_GROUP_GLOBAL,

	AP_PROPERTY,
	AP_NO_PROPERTY
};

class ClassHint {
public:
	ClassHint() : h_name("") , h_class(""), title(""), group("") { }
	~ClassHint() { }

	inline ClassHint& operator = (const ClassHint& rhs) {
		h_name = rhs.h_name;
		h_class = rhs.h_class;
		title = rhs.title;
		group = rhs.group;

		return *this;
	}
	inline bool operator == (const ClassHint& rhs) const {
		if (group.size()) {
			if (group == rhs.group)
				return true;
		} else if ((h_name == rhs.h_name) && (h_class == rhs.h_class))
			return true;
		return false;
	}

public:
	std::string h_name, h_class;
	std::string title, group;
};

class Property {
public:
	Property() : _apply_mask(0) { }
	virtual ~Property() { }

	inline RegexString& getHintName(void) { return _hint_name; }
	inline RegexString& getHintClass(void) { return _hint_class; }
	inline RegexString& getTitle(void) { return _title; }

	inline unsigned int getApplyOn(void) const { return _apply_mask; }

	inline bool isApplyOn(unsigned int mask) const { return (_apply_mask&mask); }
	inline void applyAdd(unsigned int mask) { _apply_mask |= mask; }
	inline void applyRemove(unsigned int mask) { _apply_mask &= ~mask; }

	inline std::list<unsigned int> getWsList(void) { return _ws_list; }

private:
	RegexString _hint_name, _hint_class;
	RegexString _title;

	unsigned int _apply_mask;
	std::list<unsigned int> _ws_list;
};

// AutoProperty for everything except title rewriting
class AutoProperty : public Property
{
public:
	AutoProperty() : group_size(0), group_behind(false),
									 group_focused_first(false), group_global(false),
									 _prop_mask(0) { }
	virtual ~AutoProperty() { }

	inline bool isMask(unsigned int mask) { return (_prop_mask&mask); }
	inline void maskAdd(unsigned int mask) { _prop_mask |= mask; }
	inline void maskRemove(unsigned int mask) { _prop_mask &= ~mask; }

public:
	Geometry gm;
	int gm_mask;

	bool sticky, shaded, iconified;
	bool maximized_vertical, maximized_horizontal;
	bool border, titlebar;
	unsigned int workspace, layer, skip;

	// grouping variables
	unsigned int group_size;
	std::string group_name;
	bool group_behind, group_focused_first, group_global;

private:
	unsigned int _prop_mask;
};

// TitleProperty for title rewriting
class TitleProperty : public Property
{
public:
	TitleProperty() { }
	virtual ~TitleProperty() { }

	RegexString& getTitleRule(void) { return _title_rule; }

private:
	RegexString _title_rule;
};

class AutoProperties {
public:
	AutoProperties(Config* cfg);
	~AutoProperties();

	AutoProperty* findAutoProperty(const ClassHint* class_hintbb,
																 int ws = -1, unsigned int type = 0);
	TitleProperty* findTitleProperty(const ClassHint* class_hint,
																	 unsigned int type = 0);

	void loadConfig(void);
	void unloadConfig(void);

	void removeApplyOnStart(void);

private:
	Property* findProperty(const ClassHint* class_hint,
												 std::list<Property*>* prop_list,
												 int ws, unsigned int type);

	bool parseProperty(BaseConfig::CfgSection* cs, Property* property);
	void parseAutoProperty(BaseConfig::CfgSection* cs,
												 std::list<unsigned int>* ws);
	void parseAutoGroup(BaseConfig::CfgSection* cs, AutoProperty* property);
	void parseTitleProperty(BaseConfig::CfgSection *cs);

	PropertyType getProperty(const std::string& property_name) const;
	PropertyType getGroupProperty(const std::string& property_name) const;

private:
	Config *_cfg;
	std::list<Property*> _prop_list;
	std::list<Property*> _title_prop_list;
	bool _apply_on_start;

	struct propertylist_item {
		const char *name;
		PropertyType property;

		inline bool operator == (const std::string &rhs) {
			return !(strcasecmp(name, rhs.c_str()));
		}
	};
	static propertylist_item _propertylist[];
	static propertylist_item _grouppropertylist[];
};

#endif // _AUTOPROPS_HH_
