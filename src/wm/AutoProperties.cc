//
// AutoProperties.cc for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "AutoProperties.hh"
#include "Charset.hh"
#include "Config.hh"
#include "Debug.hh"
#include "Util.hh"
#include "WinLayouter.hh"
#include "WmUtil.hh"

#include "tk/ImageHandler.hh"
#include "tk/TkGlobals.hh"

static Util::StringTo<ApplyOn> apply_on_map[] =
	{{"START", APPLY_ON_START},
	 {"NEW", APPLY_ON_NEW},
	 {"RELOAD", APPLY_ON_RELOAD},
	 {"WORKSPACE", APPLY_ON_WORKSPACE},
	 {"TRANSIENT", APPLY_ON_TRANSIENT},
	 {"TRANSIENTONLY", APPLY_ON_TRANSIENT_ONLY},
	 {nullptr, APPLY_ON_ALWAYS}};

static Util::StringTo<PropertyType> property_map[] =
	{{"WORKSPACE", AP_WORKSPACE},
	 {"PROPERTY", AP_PROPERTY},
	 {"STICKY", AP_STICKY},
	 {"SHADED", AP_SHADED},
	 {"MAXIMIZEDVERTICAL", AP_MAXIMIZED_VERTICAL},
	 {"MAXIMIZEDHORIZONTAL", AP_MAXIMIZED_HORIZONTAL},
	 {"ICONIFIED", AP_ICONIFIED},
	 {"BORDER", AP_BORDER},
	 {"TITLEBAR", AP_TITLEBAR},
	 {"FRAMEGEOMETRY", AP_FRAME_GEOMETRY},
	 {"CLIENTGEOMETRY", AP_CLIENT_GEOMETRY},
	 {"LAYER", AP_LAYER},
	 {"SKIP", AP_SKIP},
	 {"FULLSCREEN", AP_FULLSCREEN},
	 {"PLACENEW", AP_PLACE_NEW},
	 {"FOCUSNEW", AP_FOCUS_NEW},
	 {"FOCUSABLE", AP_FOCUSABLE},
	 {"CFGDENY", AP_CFG_DENY},
	 {"ALLOWEDACTIONS", AP_ALLOWED_ACTIONS},
	 {"DISALLOWEDACTIONS", AP_DISALLOWED_ACTIONS},
	 {"OPACITY", AP_OPACITY},
	 {"DECOR", AP_DECOR},
	 {"ICON", AP_ICON},
	 {"PLACEMENT", AP_PLACEMENT},
	 {nullptr, AP_NO_PROPERTY}};

static Util::StringTo<PropertyType> group_property_map[] =
	{{"SIZE", AP_GROUP_SIZE},
	 {"BEHIND", AP_GROUP_BEHIND},
	 {"FOCUSEDFIRST", AP_GROUP_FOCUSED_FIRST},
	 {"GLOBAL", AP_GROUP_GLOBAL},
	 {"RAISE", AP_GROUP_RAISE},
	 {nullptr, AP_NO_PROPERTY}};

static Util::StringTo<AtomName> window_type_map[] =
	{{"DESKTOP", WINDOW_TYPE_DESKTOP},
	 {"DOCK", WINDOW_TYPE_DOCK},
	 {"TOOLBAR", WINDOW_TYPE_TOOLBAR},
	 {"MENU", WINDOW_TYPE_MENU},
	 {"UTILITY", WINDOW_TYPE_UTILITY},
	 {"SPLASH", WINDOW_TYPE_SPLASH},
	 {"DIALOG", WINDOW_TYPE_DIALOG},
	 {"DROPDOWNMENU", WINDOW_TYPE_DROPDOWN_MENU},
	 {"POPUPMENU", WINDOW_TYPE_POPUP_MENU},
	 {"TOOLTIP", WINDOW_TYPE_TOOLTIP},
	 {"NOTIFICATION", WINDOW_TYPE_NOTIFICATION},
	 {"COMBO", WINDOW_TYPE_COMBO},
	 {"DND", WINDOW_TYPE_DND},
	 {"NORMAL", WINDOW_TYPE_NORMAL},
	 {nullptr, WINDOW_TYPE}};

Property::Property(void)
	: _apply_mask(0)
{
}

Property::~Property(void)
{
}

AutoProperty::AutoProperty(void)
	: frame_gm_mask(0),
	  client_gm_mask(0),
	  sticky(false),
	  shaded(false),
	  iconified(false),
	  maximized_vertical(false),
	  maximized_horizontal(false),
	  fullscreen(false),
	  border(true),
	  titlebar(true),
	  focusable(true),
	  place_new(true),
	  focus_new(true),
	  workspace(0),
	  skip(SKIP_NONE),
	  cfg_deny(0),
	  layer(LAYER_NORMAL),
	  focus_opacity(255),
	  unfocus_opacity(255),
	  allowed_actions(0),
	  disallowed_actions(0),
	  icon(nullptr),
	  win_layouter_types(0),
	  group_size(-1),
	  group_behind(false),
	  group_focused_first(false),
	  group_global(false),
	  group_raise(false),
	  _prop_mask(0)
{
}

AutoProperty::~AutoProperty(void)
{
	if (icon != nullptr) {
		delete icon;
	}
}

//! @brief Constructor for AutoProperties class
AutoProperties::AutoProperties(ImageHandler *image_handler)
	: _image_handler(image_handler),
	  _extended(false),
	  _harbour_sort(false),
	  _apply_on_start(true)
{
}

//! @brief Destructor for AutoProperties class
AutoProperties::~AutoProperties(void)
{
	unload();
}

/**
 * Load the user autoproperties file and then the system client autoproperties
 * file allowing the user autoproperties file to override properties from the
 * client one.
 *
 * Loading falls back to the system autoproperties file if no user file is
 * found.
 */
bool
AutoProperties::load(void)
{
	std::string cfg_file(pekwm::config()->getAutoPropsFile());
	if (! _cfg_files.requireReload(cfg_file)) {
		return false;
	}

	unload();

	bool status = true;
	CfgParser cfg(pekwm::configScriptPath());
	if (! cfg.parse(cfg_file, CfgParserSource::SOURCE_FILE, false)) {
		cfg_file = SYSCONFDIR "/autoproperties";
		if (! cfg.parse(cfg_file, CfgParserSource::SOURCE_FILE,
				false)) {
			status = false;
		}
	}

	loadRequire(cfg, cfg_file);
	load(cfg);

	// system client rules, loaded with templates always on
	cfg.clear();
	if (cfg.parse(SYSCONFDIR "/autoproperties_clientrules",
		      CfgParserSource::SOURCE_FILE, true)) {
		load(cfg);
	}

	// pekwm is dependent on type rules being defined, install default ones
	// if they have been removed from the configuration
	setDefaultTypeProperties();

	return status;
}

void
AutoProperties::load(CfgParser &cfg)
{
	// Setup template parsing if requested

	if (cfg.isDynamicContent()) {
		_cfg_files.clear();
	} else {
		_cfg_files = cfg.getCfgFiles();
	}

	// reset values
	_apply_on_start = true;

	// set load path for icons while loading auto-properties
	WithIconPath with_icon_path(pekwm::config(), _image_handler);

	CfgParser::Entry::entry_cit it(cfg.getEntryRoot()->begin());
	for (; it != cfg.getEntryRoot()->end(); ++it) {
		if (*(*it) == "PROPERTY") {
			parseAutoProperty(*it, 0);
		} else if (*(*it) == "TITLERULES") {
			parseTitleProperty(*it);
		} else if (*(*it) == "DECORRULES") {
			parseDecorProperty(*it);
		} else if (*(*it) == "TYPERULES") {
			parseTypeProperty(*it);
		} else if (*(*it) == "HARBOUR") {
			parseDockAppProperty(*it);
		} else if (*(*it) == "WORKSPACE") { // Workspace section
			parseWorkspace(*it);
		}
	}
}

/**
 * Load autoproperties quirks.
 */
void
AutoProperties::loadRequire(CfgParser &cfg, const std::string &file)
{
	// Look for requires section,
	CfgParser::Entry *section = cfg.getEntryRoot()->findSection("REQUIRE");
	if (section) {
		CfgParserKeys keys;

		keys.add_bool("TEMPLATES", _extended, false);
		section->parseKeyValues(keys.begin(), keys.end());
		keys.clear();

		// Re-load configuration with templates enabled.
		if (_extended) {
			cfg.clear(true);
			cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
		}
	} else {
		_extended = false;
	}
}

//! @brief Frees allocated memory
void
AutoProperties::unload(void)
{
	std::vector<Property*>::iterator it;

	// remove auto properties
	for (it = _prop_list.begin(); it != _prop_list.end(); ++it) {
		delete *it;
	}
	_prop_list.clear();

	// remove title properties
	for (it = _title_prop_list.begin();
	     it != _title_prop_list.end();
	     ++it) {
		delete *it;
	}
	_title_prop_list.clear();

	// remove decor properties
	for (it = _decor_prop_list.begin();
	     it != _decor_prop_list.end();
	     ++it) {
		delete *it;
	}
	_decor_prop_list.clear();

	// remove dock app properties
	for (it = _dock_app_prop_list.begin();
	     it != _dock_app_prop_list.end();
	     ++it) {
		delete *it;
	}
	_dock_app_prop_list.clear();

	// remove type properties
	std::map<AtomName, AutoProperty*>::iterator wit;
	for (wit = _window_type_prop_map.begin();
	     wit != _window_type_prop_map.end();
	     ++wit) {
		delete wit->second;
	}
	_window_type_prop_map.clear();
}

//! @brief Finds a property from the prop_list
Property*
AutoProperties::findProperty(const ClassHint& class_hint,
			     std::vector<Property*>* prop_list,
			     int ws, ApplyOn type)
{
	// Allready remove apply on start
	if (! _apply_on_start && (type == APPLY_ON_START))
		return nullptr;

	// start searching for a suitable property
	std::vector<Property*>::iterator it = prop_list->begin();
	for (; it != prop_list->end(); ++it) {
		// see if the type matches, if we have one
		if ((type != APPLY_ON_ALWAYS) && ! (*it)->isApplyOn(type))
			continue;

		if (matchAutoClass(class_hint, *it)) {
			return (*it)->applyOnWs(ws) ? *it : nullptr;
		}
	}

	return nullptr;
}

/**
 * Parse regex_str and set on regex, outputting warning with name if
 * it fails.
 */
bool
AutoProperties::parseRegexpOrWarning(RegexString &regex,
				     const std::string &regex_str,
				     const std::string &name)
{
	if (! regex_str.size() || regex.parse_match(regex_str)) {
		return true;
	} else {
		USER_WARN("invalid regexp " << regex_str
			  << " for autoproperty " << name);
		return false;
	}
}

//! @brief Parses a property match rule
//! @param str String to parse.
//! @param prop Property to place result in.
//! @return true on success, else false.
bool
AutoProperties::parsePropertyMatch(const std::string &str, Property *prop)
{
	// Format of property matches are regexp,regexp . Split up in class
	// and role regexps.
	std::vector<std::string> tokens;
	Util::splitString(str, tokens, ",", _extended ? 5 : 2, true);

	bool status = false;
	if (tokens.size() >= 2) {
		// Make sure one of the two regexps compiles
		status = parseRegexpOrWarning(prop->getHintName(),
					      tokens[0], "name");
		status = status
			&& parseRegexpOrWarning(prop->getHintClass(),
						tokens[1], "class");
	}

	// Parse extended part of regexp, role, title and apply on
	if (status && _extended) {
		if (tokens.size() > 2) {
			status = parseRegexpOrWarning(prop->getRole(),
						      tokens[2], "role");
		}
		if (status && tokens.size() > 3) {
			status = parseRegexpOrWarning(prop->getTitle(),
						      tokens[3], "title");
		}
		if (status && tokens.size() > 4) {
			parsePropertyApplyOn(tokens[4], prop);
		}
	}

	return status;
}

//! @brief Parses a cs and sets up a basic property
bool
AutoProperties::parseProperty(CfgParser::Entry *section, Property *prop)
{
	// Get extra matching info.
	CfgParser::Entry *value = section->findEntry("TITLE");
	if (value) {
		parseRegexpOrWarning(prop->getTitle(), value->getValue(),
				     "title");
	}
	value = section->findEntry("ROLE");
	if (value) {
		parseRegexpOrWarning(prop->getRole(), value->getValue(),
				     "role");
	}

	// Parse apply on mask.
	value = section->findEntry("APPLYON");
	if (value) {
		parsePropertyApplyOn(value->getValue(), prop);
	}

	return true;
}

/**
 * Parse property apply on.
 */
void
AutoProperties::parsePropertyApplyOn(const std::string &apply_on,
				     Property *prop)
{
	std::vector<std::string> tokens;
	if (Util::splitString(apply_on, tokens, " \t", 5)) {
		std::vector<std::string>::iterator it = tokens.begin();
		for (; it != tokens.end(); ++it) {
			int num = Util::StringToGet(apply_on_map, *it);
			prop->applyAdd(static_cast<unsigned int>(num));
		}
	}
}

void
AutoProperties::parseAutoProperty(CfgParser::Entry *section,
				  std::vector<uint> *ws)
{
	// Get sub section
	section = section->getSection();
	if (! section) {
		return;
	}

	AutoProperty *property = new AutoProperty();
	parsePropertyMatch(section->getValue(), property);

	if (parseProperty(section, property)) {
		parseAutoPropertyValue(section, property, ws);
		_prop_list.push_back(property);
	} else {
		delete property;
	}
}

/**
 * Parse the Group section of an auto property.
 */
void
AutoProperties::parseAutoGroup(CfgParser::Entry *section,
			       AutoProperty* property)
{
	if (! section) {
		return;
	}

	if (section->getValue().size()) {
		property->group_name = section->getValue();
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		PropertyType property_type =
			Util::StringToGet(group_property_map,
					  (*it)->getName());

		switch (property_type) {
		case AP_GROUP_SIZE:
			property->group_size = std::stoi((*it)->getValue());
			break;
		case AP_GROUP_BEHIND:
			property->group_behind =
				Util::isTrue((*it)->getValue());
			break;
		case AP_GROUP_FOCUSED_FIRST:
			property->group_focused_first =
				Util::isTrue((*it)->getValue());
			break;
		case AP_GROUP_GLOBAL:
			property->group_global =
				Util::isTrue((*it)->getValue());
			break;
		case AP_GROUP_RAISE:
			property->group_raise = Util::isTrue((*it)->getValue());
			break;
		default:
			// do nothing
			break;
		}
	}
}

/**
 * Parses a title property section.
 */
void
AutoProperties::parseTitleProperty(CfgParser::Entry *section)
{
	section = section->getSection();

	TitleProperty *title_property;
	CfgParser::Entry *title_section;

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		title_section = (*it)->getSection();
		if (! title_section) {
			continue;
		}

		title_property = new TitleProperty();
		parsePropertyMatch(title_section->getValue(), title_property);
		if (parseProperty(title_section, title_property)) {
			CfgParser::Entry *value =
				title_section->findEntry("RULE");
			if (value && title_property->getTitleRule()
					.parse_ed_s(value->getValue())) {
				_title_prop_list.push_back(title_property);
				title_property = 0;
			}
		}

		if (title_property) {
			delete title_property;
		}
	}
}

/**
 * Parse decor property sections.
 */
void
AutoProperties::parseDecorProperty(CfgParser::Entry *section)
{
	section = section->getSection();

	DecorProperty *decor_property;
	CfgParser::Entry *decor_section;

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		decor_section = (*it)->getSection ();
		if (! decor_section) {
			continue;
		}

		decor_property = new DecorProperty();
		parsePropertyMatch(decor_section->getValue (), decor_property);
		if (parseProperty(decor_section, decor_property)) {
			CfgParser::Entry *value =
				decor_section->findEntry("DECOR");
			if (value) {
				decor_property->applyAdd(APPLY_ON_START);
				decor_property->setName(value->getValue());
				_decor_prop_list.push_back(decor_property);
				decor_property = 0;
			}
		}

		if (decor_property) {
			delete decor_property;
		}
	}
}

/**
 * Parse dock app properties.
 */
void
AutoProperties::parseDockAppProperty(CfgParser::Entry *section)
{
	section = section->getSection();

	// Reset harbour sort, set to true if POSITION property found.
	_harbour_sort = false;

	DockAppProperty *dock_property;
	CfgParser::Entry *dock_section;

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		dock_section = (*it)->getSection();
		if (! dock_section) {
			continue;
		}

		dock_property = new DockAppProperty();
		parsePropertyMatch(dock_section->getValue(), dock_property);
		if (parseProperty(dock_section, dock_property)) {
			CfgParser::Entry *value =
				dock_section->findEntry("POSITION");
			if (value) {
				_harbour_sort = true;

				int position = std::stoi(value->getValue());
				dock_property->setPosition(position);
				_decor_prop_list.push_back(dock_property);
				dock_property = 0;
			}
		}

		if (dock_property) {
			delete dock_property;
		}
	}
}

void
AutoProperties::parseWorkspace(CfgParser::Entry* section)
{
	CfgParser::Entry *workspace = section->getSection();
	std::vector<std::string> tokens;
	std::vector<std::string>::iterator it;
	if (Util::splitString(workspace->getValue(), tokens, " \t")) {
		std::vector<uint> workspaces;
		for (it = tokens.begin(); it != tokens.end(); ++it) {
			workspaces.push_back(std::stoi(*it));
		}

		// Get all properties on for these workspaces.
		CfgParser::Entry::entry_cit workspace_it(workspace->begin());
		for (; workspace_it != workspace->end(); ++workspace_it) {
			parseAutoProperty(*workspace_it, &workspaces);
		}
	}
}

//! @brief Parse type auto properties.
//! @param section Section containing properties.
void
AutoProperties::parseTypeProperty(CfgParser::Entry *section)
{
	// Get sub section
	section = section->getSection();

	AtomName atom;
	AutoProperty *type_property;
	std::map<AtomName, AutoProperty*>::iterator atom_it;

	// Look for all type properties
	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		CfgParser::Entry *type_section = (*it)->getSection();
		if (! type_section) {
			continue;
		}

		// Create new property and try to parse
		type_property = new AutoProperty();
		atom = Util::StringToGet(window_type_map,
					 type_section->getValue());
		if (atom == WINDOW_TYPE) {
			USER_WARN("unknown type " << type_section->getValue()
				  << " for autoproperty");
		}

		if (atom != WINDOW_TYPE
		    && parseProperty(type_section, type_property)) {
			// Parse of match ok, parse values
			parseAutoPropertyValue(type_section, type_property, 0);

			// Add to list, make sure it does not exist already
			atom_it = _window_type_prop_map.find(atom);
			if (atom_it != _window_type_prop_map.end()) {
				USER_WARN("multiple type autoproperties for "
					  "type " << type_section->getValue());
				delete atom_it->second;
			}
			_window_type_prop_map[atom] = type_property;
		} else {
			delete type_property;
		}
	}
}

//! @brief Set default values for type auto properties not in configuration.
void
AutoProperties::setDefaultTypeProperties(void)
{
	// DESKTOP
	if (! findWindowTypeProperty(WINDOW_TYPE_DESKTOP)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_CLIENT_GEOMETRY);
		prop->client_gm_mask =
			X11::parseGeometry("0x0+0+0", prop->client_gm);
		prop->maskAdd(AP_STICKY);
		prop->sticky = true;
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = false;
		prop->maskAdd(AP_BORDER);
		prop->border = false;
		prop->maskAdd(AP_SKIP);
		prop->skip =
			SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP|SKIP_PAGER
			|SKIP_TASKBAR;
		prop->maskAdd(AP_LAYER);
		prop->layer = LAYER_DESKTOP;
		prop->maskAdd(AP_FOCUSABLE);
		prop->focusable = false;
		prop->maskAdd(AP_DISALLOWED_ACTIONS);
		prop->disallowed_actions =
			ACTION_ACCESS_MOVE|ACTION_ACCESS_RESIZE;

		_window_type_prop_map[WINDOW_TYPE_DESKTOP] = prop;
	}

	// DOCK
	if (! findWindowTypeProperty(WINDOW_TYPE_DOCK)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_STICKY);
		prop->sticky = true;
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = false;
		prop->maskAdd(AP_BORDER);
		prop->border = false;
		prop->maskAdd(AP_SKIP);
		prop->skip =
			SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_PAGER|SKIP_TASKBAR;
		prop->maskAdd(AP_LAYER);
		prop->layer = LAYER_DOCK;
		prop->maskAdd(AP_FOCUSABLE);
		prop->focusable = false;
		prop->maskAdd(AP_DISALLOWED_ACTIONS);
		prop->disallowed_actions =
			ACTION_ACCESS_MOVE|ACTION_ACCESS_RESIZE;

		_window_type_prop_map[WINDOW_TYPE_DOCK] = prop;
	}

	// TOOLBAR
	if (! findWindowTypeProperty(WINDOW_TYPE_TOOLBAR)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = true;
		prop->maskAdd(AP_BORDER);
		prop->border = true;
		prop->maskAdd(AP_SKIP);
		prop->skip =
			SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_PAGER|SKIP_TASKBAR;

		_window_type_prop_map[WINDOW_TYPE_TOOLBAR] = prop;
	}

	// MENU
	if (! findWindowTypeProperty(WINDOW_TYPE_MENU)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = false;
		prop->maskAdd(AP_BORDER);
		prop->border = false;
		prop->maskAdd(AP_SKIP);
		prop->skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP|SKIP_PAGER
			|SKIP_TASKBAR;

		_window_type_prop_map[WINDOW_TYPE_MENU] = prop;
	}

	// UTILITY
	if (! findWindowTypeProperty(WINDOW_TYPE_UTILITY)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = true;
		prop->maskAdd(AP_BORDER);
		prop->border = true;
		prop->maskAdd(AP_SKIP);
		prop->skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP;

		_window_type_prop_map[WINDOW_TYPE_UTILITY] = prop;
	}

	// SPLASH
	if (! findWindowTypeProperty(WINDOW_TYPE_SPLASH)) {
		AutoProperty *prop = new AutoProperty();
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = false;
		prop->maskAdd(AP_BORDER);
		prop->border = false;

		_window_type_prop_map[WINDOW_TYPE_SPLASH] = prop;
	}

	// DIALOG
	if (! findWindowTypeProperty(WINDOW_TYPE_DIALOG)) {
		AutoProperty *prop = new AutoProperty();
		_window_type_prop_map[WINDOW_TYPE_DIALOG] = prop;
	}

	// DROPDOWNMENU
	if (! findWindowTypeProperty(WINDOW_TYPE_DROPDOWN_MENU)) {
		AutoProperty *prop = new AutoProperty();
		_window_type_prop_map[WINDOW_TYPE_DROPDOWN_MENU] = prop;
	}

	// POPUPMENU
	if (! findWindowTypeProperty(WINDOW_TYPE_POPUP_MENU)) {
		AutoProperty *prop = new AutoProperty();
		_window_type_prop_map[WINDOW_TYPE_POPUP_MENU] = prop;
	}

	// TOOLTIP
	if (! findWindowTypeProperty(WINDOW_TYPE_TOOLTIP)) {
		AutoProperty *prop = new AutoProperty();
		prop->titlebar = false;
		prop->maskAdd(AP_TITLEBAR);
		prop->border = false;
		prop->maskAdd(AP_BORDER);
		prop->focus_new = false;
		prop->maskAdd(AP_FOCUS_NEW);
		prop->skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP;
		prop->maskAdd(AP_SKIP);
		_window_type_prop_map[WINDOW_TYPE_TOOLTIP] = prop;
	}

	// NOTIFICATION
	if (! findWindowTypeProperty(WINDOW_TYPE_NOTIFICATION)) {
		AutoProperty *prop = new AutoProperty();
		prop->titlebar = false;
		prop->maskAdd(AP_TITLEBAR);
		prop->border = false;
		prop->maskAdd(AP_BORDER);
		prop->focus_new = false;
		prop->maskAdd(AP_FOCUS_NEW);
		prop->skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP;
		prop->maskAdd(AP_SKIP);
		_window_type_prop_map[WINDOW_TYPE_NOTIFICATION] = prop;
	}

	// COMBO
	if (! findWindowTypeProperty(WINDOW_TYPE_COMBO)) {
		AutoProperty *prop = new AutoProperty();
		_window_type_prop_map[WINDOW_TYPE_COMBO] = prop;
	}

	// DND
	if (! findWindowTypeProperty(WINDOW_TYPE_DND)) {
		AutoProperty *prop = new AutoProperty();
		_window_type_prop_map[WINDOW_TYPE_DND] = prop;
	}
}

//! @brief Parse AutoProperty value attributes.
//! @param section Config section to parse.
//! @param prop Property to store result in.
//! @param ws List of workspaces to apply property on.
void
AutoProperties::parseAutoPropertyValue(CfgParser::Entry *section,
				       AutoProperty *prop,
				       std::vector<uint> *ws)
{
	// Copy workspaces, if any
	if (ws) {
		prop->setWorkspaces(*ws);
	}

	// See if we have a group section
	CfgParser::Entry *group_section(section->findSection("GROUP"));
	if (group_section) {
		parseAutoGroup(group_section, prop);
	}

	// start parsing of values
	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		parseAutoPropertyType(*it, prop);
	}
}

void
AutoProperties::parseAutoPropertyType(CfgParser::Entry* it, AutoProperty* prop)
{
	std::vector<std::string> tokens;
	std::vector<std::string>::iterator token_it;
	PropertyType property_type =
		Util::StringToGet(property_map, it->getName());

	switch (property_type) {
	case AP_STICKY:
		prop->maskAdd(AP_STICKY);
		prop->sticky = Util::isTrue(it->getValue());
		break;
	case AP_SHADED:
		prop->maskAdd(AP_SHADED);
		prop->shaded = Util::isTrue(it->getValue());
		break;
	case AP_MAXIMIZED_VERTICAL:
		prop->maskAdd(AP_MAXIMIZED_VERTICAL);
		prop->maximized_vertical = Util::isTrue(it->getValue());
		break;
	case AP_MAXIMIZED_HORIZONTAL:
		prop->maskAdd(AP_MAXIMIZED_HORIZONTAL);
		prop->maximized_horizontal = Util::isTrue(it->getValue());
		break;
	case AP_ICONIFIED:
		prop->maskAdd(AP_ICONIFIED);
		prop->iconified = Util::isTrue(it->getValue());
		break;
	case AP_BORDER:
		prop->maskAdd(AP_BORDER);
		prop->border = Util::isTrue(it->getValue());
		break;
	case AP_TITLEBAR:
		prop->maskAdd(AP_TITLEBAR);
		prop->titlebar = Util::isTrue(it->getValue());
		break;
	case AP_FRAME_GEOMETRY:
		prop->maskAdd(AP_FRAME_GEOMETRY);
		prop->frame_gm_mask =
			X11::parseGeometry(it->getValue(), prop->frame_gm);
		break;
	case AP_CLIENT_GEOMETRY:
		prop->maskAdd(AP_CLIENT_GEOMETRY);
		prop->client_gm_mask =
			X11::parseGeometry(it->getValue(), prop->client_gm);
		break;
	case AP_LAYER:
		prop->layer = ActionConfig::getLayer(it->getValue());
		if (prop->layer != LAYER_NONE) {
			prop->maskAdd(AP_LAYER);
		}
		break;
	case AP_WORKSPACE:
		try {
			prop->workspace = std::stoi(it->getValue()) - 1;
			prop->maskAdd(AP_WORKSPACE);
		} catch (std::invalid_argument&) {
		}
		break;
	case AP_SKIP:
		prop->maskAdd(AP_SKIP);
		tokens.clear();
		if ((Util::splitString(it->getValue(), tokens, " \t"))) {
			for (token_it = tokens.begin();
			     token_it != tokens.end();
			     ++token_it) {
				prop->skip |= ActionConfig::getSkip(*token_it);
			}
		}
		break;
	case AP_FULLSCREEN:
		prop->maskAdd(AP_FULLSCREEN);
		prop->fullscreen = Util::isTrue(it->getValue());
		break;
	case AP_PLACE_NEW:
		prop->maskAdd(AP_PLACE_NEW);
		prop->place_new = Util::isTrue(it->getValue());
		break;
	case AP_FOCUS_NEW:
		prop->maskAdd(AP_FOCUS_NEW);
		prop->focus_new = Util::isTrue(it->getValue());
		break;
	case AP_FOCUSABLE:
		prop->maskAdd(AP_FOCUSABLE);
		prop->focusable = Util::isTrue(it->getValue());
		break;
	case AP_CFG_DENY:
		prop->maskAdd(AP_CFG_DENY);
		tokens.clear();
		if ((Util::splitString(it->getValue(), tokens, " \t"))) {
			for (token_it = tokens.begin();
			     token_it != tokens.end();
			     ++token_it) {
				prop->cfg_deny |= ActionConfig::getCfgDeny(
						*token_it);
			}
		}
		break;
	case AP_ALLOWED_ACTIONS:
		prop->maskAdd(AP_ALLOWED_ACTIONS);
		pekwm::config()->parseActionAccessMask(
				it->getValue(),
				prop->allowed_actions);
		break;
	case AP_DISALLOWED_ACTIONS:
		prop->maskAdd(AP_DISALLOWED_ACTIONS);
		pekwm::config()->parseActionAccessMask(
				it->getValue(),
				prop->disallowed_actions);
		break;
	case AP_OPACITY:
		prop->maskAdd(AP_OPACITY);
		Config::parseOpacity(it->getValue(),
				     prop->focus_opacity,
				     prop->unfocus_opacity);
		break;
	case AP_DECOR:
		prop->maskAdd(AP_DECOR);
		prop->frame_decor = it->getValue();
		break;
	case AP_ICON: {
		PImage *image =
			_image_handler->getImage(it->getValue());
		if (image == nullptr) {
			USER_WARN("failed to load icon " << it->getValue()
				  << " ignoring icon property");
		} else {
			prop->maskAdd(AP_ICON);
			prop->icon = new PImageIcon(image);
			_image_handler->returnImage(image);
		}
		break;
	}
	case AP_PLACEMENT:
		prop->maskAdd(AP_PLACEMENT);
		prop->win_layouter_types = parsePlacement(it->getValue());
	      break;
	default:
		// do nothing
		break;
	}
}

//! @brief Searches the _prop_list for a property
AutoProperty*
AutoProperties::findAutoProperty(const ClassHint& class_hint, int ws,
				 ApplyOn type)
{
	return static_cast<AutoProperty*>(
			findProperty(class_hint, &_prop_list, ws, type));
}

//! @brief Searches the _title_prop_list for a property
TitleProperty*
AutoProperties::findTitleProperty(const ClassHint& class_hint)
{
	return static_cast<TitleProperty*>(
			findProperty(class_hint, &_title_prop_list, -1,
				     APPLY_ON_ALWAYS));
}

DecorProperty*
AutoProperties::findDecorProperty(const ClassHint& class_hint)
{
	return static_cast<DecorProperty*>(
			findProperty(class_hint, &_decor_prop_list, -1,
				     APPLY_ON_ALWAYS));
}

DockAppProperty*
AutoProperties::findDockAppProperty(const ClassHint& class_hint)
{
	return static_cast<DockAppProperty*>(
			findProperty(class_hint, &_dock_app_prop_list, -1,
				     APPLY_ON_ALWAYS));
}

//! @brief Get AutoProperty for window of type type
//! @param atom Atom to get property for.
//! @return AutoProperty on success, else 0.
AutoProperty*
AutoProperties::findWindowTypeProperty(AtomName atom)
{
	AutoProperty *prop = 0;
	std::map<AtomName, AutoProperty*>::iterator it
		= _window_type_prop_map.find(atom);
	if (it != _window_type_prop_map.end()) {
		prop = it->second;
	}

	return prop;
}

//! @brief Removes all ApplyOnStart actions as they consume memory
void
AutoProperties::removeApplyOnStart(void)
{
	std::vector<Property*>::iterator it = _prop_list.begin();
	for (; it != _prop_list.end(); ++it) {
		if ((*it)->isApplyOn(APPLY_ON_START)) {
			(*it)->applyRemove(APPLY_ON_START);
			if (! (*it)->getApplyOn()) {
				delete *it;
				it = _prop_list.erase(it);
				--it; // compensate for the ++ in the loop
			}
		}
	}

	_apply_on_start = false;
}

//! @brief Tries to match a class hint against an autoproperty data entry
bool
AutoProperties::matchAutoClass(const ClassHint& hint, Property *prop)
{
	bool ok = false;

	if ((prop->getHintName() == hint.h_name)
	    && (prop->getHintClass() == hint.h_class))
		{
			ok = true;
			if (prop->getTitle ().is_match_ok ())  {
				ok = (prop->getTitle () == hint.title);
			}
			if (ok && prop->getRole ().is_match_ok ()) {
				ok = (prop->getRole () == hint.h_role);
			}
		}

	return ok;
}

/**
 * Parse list of placement types separated by string.
 */
int
AutoProperties::parsePlacement(const std::string &value)
{
	int placement = 0;
	std::vector<std::string> toks;
	Util::splitString(value, toks, " \t");
	if (toks.size() > WIN_LAYOUTER_MASK_NUM) {
		toks.resize(WIN_LAYOUTER_MASK_NUM);
	}

	std::vector<std::string>::iterator it(toks.begin());
	for (int s = 0; it != toks.end(); ++it, s += WIN_LAYOUTER_SHIFT) {
		placement |= win_layouter_type_from_string(*it) << s;
	}
	return placement;
}
