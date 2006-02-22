//
// AutoProperties.cc for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
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
using std::find;

AutoProperties *AutoProperties::_instance = NULL;

//! @brief Constructor for AutoProperties class
AutoProperties::AutoProperties(void) :
#ifdef HARBOUR
_harbour_sort(false),
#endif // HARBOUR
_apply_on_start(true)
{
#ifdef DEBUG
  if (_instance)
    {
      cerr << __FILE__ << "@" << __LINE__ << ": "
           << "AutoProperties(" << this << ")::AutoProperties()" << endl
           << " *** _instance allready set: " << _instance << endl;
    }
#endif // DEBUG
  _instance = this;

  // fill parsing maps
  _apply_on_map[""] = APPLY_ON_NONE;
  _apply_on_map["START"] = APPLY_ON_START;
  _apply_on_map["NEW"] = APPLY_ON_NEW;
  _apply_on_map["RELOAD"] = APPLY_ON_RELOAD;
  _apply_on_map["WORKSPACE"] = APPLY_ON_WORKSPACE;
  _apply_on_map["TRANSIENT"] = APPLY_ON_TRANSIENT;
  _apply_on_map["TRANSIENTONLY"] = APPLY_ON_TRANSIENT_ONLY;

  // global properties
  _property_map[""] = AP_NO_PROPERTY;
  _property_map["WORKSPACE"] = AP_WORKSPACE;
  _property_map["PROPERTY"] = AP_PROPERTY;
  _property_map["STICKY"] = AP_STICKY;
  _property_map["SHADED"] = AP_SHADED;
  _property_map["MAXIMIZEDVERTICAL"] = AP_MAXIMIZED_VERTICAL;
  _property_map["MAXIMIZEDHORIZONTAL"] = AP_MAXIMIZED_HORIZONTAL;
  _property_map["ICONIFIED"] = AP_ICONIFIED;
  _property_map["BORDER"] = AP_BORDER;
  _property_map["TITLEBAR"] = AP_TITLEBAR;
  _property_map["FRAMEGEOMETRY"] = AP_FRAME_GEOMETRY;
  _property_map["CLIENTGEOMETRY"] = AP_CLIENT_GEOMETRY;
  _property_map["LAYER"] = AP_LAYER;
  _property_map["SKIP"] = AP_SKIP;
  _property_map["FULLSCREEN"] = AP_FULLSCREEN;
  _property_map["PLACENEW"] = AP_PLACE_NEW;
  _property_map["FOCUSNEW"] = AP_FOCUS_NEW;
  _property_map["FOCUSABLE"] = AP_FOCUSABLE;
  _property_map["VIEWPORT"] = AP_VIEWPORT;
  _property_map["CFGDENY"] = AP_CFG_DENY;

  // group properties
  _group_property_map[""] = AP_NO_PROPERTY;
  _group_property_map["SIZE"] = AP_GROUP_SIZE;
  _group_property_map["BEHIND"] = AP_GROUP_BEHIND;
  _group_property_map["FOCUSEDFIRST"] = AP_GROUP_FOCUSED_FIRST;
  _group_property_map["GLOBAL"] = AP_GROUP_GLOBAL;
  _group_property_map["RAISE"] = AP_GROUP_RAISE;
}

//! @brief Destructor for AutoProperties class
AutoProperties::~AutoProperties(void)
{
	unload();
	_instance = NULL;
}

//! @brief Loads the autoprop config file.
void
AutoProperties::load(void)
{
  // dealloc memory
  unload();

  CfgParser a_cfg;
  if (!a_cfg.parse (Config::instance()->getAutoPropsFile()))
    {
      string cfg_file = SYSCONFDIR "/autoproperties";
      if (!a_cfg.parse (cfg_file))
        return;
    }

  // reset values
  _apply_on_start = true;

  vector<string> tokens;
  vector<string>::iterator it;
  list<uint> workspaces;

  CfgParser::Entry *op_it (a_cfg.get_entry_root ());
  while ((op_it = op_it->get_section_next ()) != NULL)
    {
      if (*op_it == "PROPERTY")
        parseAutoProperty (op_it, NULL);
      else if (*op_it == "TITLERULES")
        parseTitleProperty (op_it);
      else if (*op_it == "DECORRULES")
        parseDecorProperty (op_it);
#ifdef HARBOUR
      else if (*op_it == "HARBOUR")
        parseDockAppProperty (op_it);
#endif // HARBOUR
      else if (*op_it == "WORKSPACE") // Workspace section
        {
          op_it = op_it->get_section ();

          CfgParser::Entry *op_value = op_it->find_entry ("WORKSPACE");
          if (!op_value)
            continue; // Need workspace numbers.

          tokens.clear();
          if (Util::splitString(op_value->get_value (), tokens, " \t"))
            {
              workspaces.clear();
              for (it = tokens.begin(); it != tokens.end(); ++it)
                workspaces.push_back(strtol(it->c_str(), NULL, 10) - 1);

              // Get all properties on for these workspaces.
              CfgParser::Entry *op_it_sub (op_it->get_section ());
              while ((op_it_sub = op_it_sub->get_section_next ()) != NULL)
                parseAutoProperty(op_it_sub, &workspaces);
            }
        }
    }
}

//! @brief Frees allocated memory
void
AutoProperties::unload(void)
{
	list<Property*>::iterator it;

	// remove auto properties
	for (it = _prop_list.begin(); it != _prop_list.end(); ++it) {
		delete *it;
	}
	_prop_list.clear();		

	// remove title properties
	for (it = _title_prop_list.begin(); it != _title_prop_list.end(); ++it) {
		delete *it;
	}
	_title_prop_list.clear();

	// remove decor properties
	for (it = _decor_prop_list.begin(); it != _decor_prop_list.end(); ++it) {
		delete *it;
	}
	_decor_prop_list.clear();
}

//! @brief Finds a property from the prop_list
Property*
AutoProperties::findProperty(const ClassHint* class_hint,
														 std::list<Property*>* prop_list, int ws, uint type)
{
	// Allready remove apply on start
	if (!_apply_on_start && (type == APPLY_ON_START))
		return NULL;

	list<Property*>::iterator it(prop_list->begin());
	list<uint>::iterator w_it;

	// start searching for a suitable property
	for (bool ok = false; it != prop_list->end(); ++it, ok = false) {
		// see if the type matches, if we have one
		if ((type != 0) && !(*it)->isApplyOn(type))
				continue;

		if (matchAutoClass(*class_hint, *it)) {

			// make sure it applies on the correct workspace
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

//! @brief Parses a cs and sets up a basic property
bool
AutoProperties::parseProperty(CfgParser::Entry *op_section, Property *prop)
{
  vector<string> tokens;
  if ((Util::splitString (op_section->get_value (), tokens, ",", 2)) != 2)
    return false;

  CfgParser::Entry *op_value;

  if (!prop->getHintName().parse_match (tokens[0])
      || !prop->getHintClass().parse_match (tokens[1]))
    {
      return false;
    }

  // Get extra matching info.
  op_value = op_section->find_entry ("TITLE");
  if (op_value)
    prop->getTitle().parse_match (op_value->get_value ());
  op_value = op_section->find_entry ("ROLE");
  if (op_value)
    prop->getRole().parse_match (op_value->get_value ());

  // Parse apply on mask.
  op_value = op_section->find_entry ("APPLYON");
  if (op_value)
    {
      tokens.clear();
      if ((Util::splitString(op_value->get_value (), tokens, " \t", 5)))
        {
          vector<string>::iterator it;
          for (it = tokens.begin(); it != tokens.end(); ++it)
            prop->applyAdd((uint) ParseUtil::getValue<ApplyOn>(*it, _apply_on_map));
        }
    }

  return true;
}

//! @brief Parses AutopProperty
void
AutoProperties::parseAutoProperty(CfgParser::Entry *op_section,
                                  std::list<uint>* ws)
{
  op_section = op_section->get_section ();

  AutoProperty* property = new AutoProperty();
  if (!parseProperty(op_section, property))
    {
      delete property;
      return; // Not a valid property.
    }

  // copy workspaces, if any
  if (ws)
    property->getWsList().assign(ws->begin(), ws->end());

  // see if we have a group section
  CfgParser::Entry *op_it(op_section->find_section ("GROUP"));
  if (op_it)
    parseAutoGroup(op_it, property);

  // start parsing of values
  string name, value;
  vector<string> tokens;
  vector<string>::iterator it;

  PropertyType property_type;
  for (op_it = op_section->get_entry_next (); op_it; op_it = op_it->get_entry_next ())
    {
      property_type = ParseUtil::getValue<PropertyType>(op_it->get_name (),
                                                        _property_map);

      switch (property_type) {
      case AP_STICKY:
        property->maskAdd(AP_STICKY);
        property->sticky = Util::isTrue(op_it->get_value ());
        break;
      case AP_SHADED:
        property->maskAdd(AP_SHADED);
        property->shaded = Util::isTrue(op_it->get_value ());
        break;
      case AP_MAXIMIZED_VERTICAL:
        property->maskAdd(AP_MAXIMIZED_VERTICAL);
        property->maximized_vertical = Util::isTrue(op_it->get_value ());
        break;
      case AP_MAXIMIZED_HORIZONTAL:
        property->maskAdd(AP_MAXIMIZED_HORIZONTAL);
        property->maximized_horizontal = Util::isTrue(op_it->get_value ());
        break;
      case AP_ICONIFIED:
        property->maskAdd(AP_ICONIFIED);
        property->iconified = Util::isTrue(op_it->get_value ());
        break;
      case AP_BORDER:
        property->maskAdd(AP_BORDER);
        property->border = Util::isTrue(op_it->get_value ());
        break;
      case AP_TITLEBAR:
        property->maskAdd(AP_TITLEBAR);
        property->titlebar = Util::isTrue(op_it->get_value ());
        break;
      case AP_FRAME_GEOMETRY:
        property->maskAdd(AP_FRAME_GEOMETRY);
        property->frame_gm_mask =
          XParseGeometry((char*) op_it->get_value ().c_str(),
                         &property->frame_gm.x, &property->frame_gm.y,
                         &property->frame_gm.width, &property->frame_gm.height);
        break;
      case AP_CLIENT_GEOMETRY:
        property->maskAdd(AP_CLIENT_GEOMETRY);
        property->client_gm_mask =
          XParseGeometry((char*) op_it->get_value ().c_str(),
                         &property->client_gm.x, &property->client_gm.y,
                         &property->client_gm.width, &property->client_gm.height);
        break;
      case AP_LAYER:
        property->maskAdd(AP_LAYER);
        property->layer = Config::instance()->getLayer(op_it->get_value ());
        break;
      case AP_WORKSPACE:
        property->maskAdd(AP_WORKSPACE);
        property->workspace = unsigned(strtol(op_it->get_value ().c_str(), NULL, 10) - 1);
        break;
      case AP_SKIP:
        property->maskAdd(AP_SKIP);
        tokens.clear();
        if ((Util::splitString(op_it->get_value (), tokens, " \t"))) {
          for (it = tokens.begin(); it != tokens.end(); ++it)
            property->skip |= Config::instance()->getSkip(*it);
        }
        break;
      case AP_FULLSCREEN:
        property->maskAdd(AP_FULLSCREEN);
        property->fullscreen = Util::isTrue(op_it->get_value ());
        break;
      case AP_PLACE_NEW:
        property->maskAdd(AP_PLACE_NEW);
        property->place_new = Util::isTrue(op_it->get_value ());
        break;
      case AP_FOCUS_NEW:
        property->maskAdd(AP_FOCUS_NEW);
        property->focus_new = Util::isTrue(op_it->get_value ());
        break;
      case AP_FOCUSABLE:
        property->maskAdd(AP_FOCUSABLE);
        property->focusable = Util::isTrue(op_it->get_value ());
        break;
      case AP_CFG_DENY:
        property->maskAdd(AP_CFG_DENY);
        tokens.clear();
        if ((Util::splitString(op_it->get_value (), tokens, " \t"))) {
          for (it = tokens.begin(); it != tokens.end(); ++it) {
            property->cfg_deny |= Config::instance()->getCfgDeny(*it);
          }
        }
        break;
      case AP_VIEWPORT:
        tokens.clear();
        if (Util::splitString(op_it->get_value (), tokens, " \t", 2) == 2) {
          property->maskAdd(AP_VIEWPORT);
          property->viewport_col = strtol(tokens[0].c_str(), NULL, 10) - 1;
          property->viewport_row = strtol(tokens[1].c_str(), NULL, 10) - 1;
        }
        break;

      default:
        // do nothing
        break;
      }
    }

	_prop_list.push_back(property);
}

//! @brief Parses a Group section of the AutoProps
void
AutoProperties::parseAutoGroup(CfgParser::Entry *op_section,
                               AutoProperty* property)
{
  op_section = op_section->get_section ();

  if (op_section->get_value ().size ())
      property->group_name = op_section->get_value ();

  PropertyType property_type;
  while ((op_section = op_section->get_entry_next ()) != NULL)
    {
      property_type = ParseUtil::getValue<PropertyType> (op_section->get_name (), _group_property_map);

      switch (property_type)
        {
        case AP_GROUP_SIZE:
          property->group_size = strtol (op_section->get_value ().c_str (),
                                         NULL, 10);
          break;
        case AP_GROUP_BEHIND:
          property->group_behind = Util::isTrue (op_section->get_value ());
          break;
        case AP_GROUP_FOCUSED_FIRST:
          property->group_focused_first = Util::isTrue (op_section->get_value ());
          break;
        case AP_GROUP_GLOBAL:
          property->group_global = Util::isTrue (op_section->get_value ());
          break;
        case AP_GROUP_RAISE:
          property->group_raise = Util::isTrue (op_section->get_value ());
          break;
        default:
          // do nothing
          break;
        }
    }
}

//! @brief Parses a title property section
void
AutoProperties::parseTitleProperty(CfgParser::Entry *op_section)
{
  op_section = op_section->get_section ();

  bool ok;
  TitleProperty *property;
  CfgParser::Entry *op_sub;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      op_sub = op_section->get_section ();

      ok = false;
      property = new TitleProperty();

      if (parseProperty (op_sub, property))
        {
          CfgParser::Entry *op_value = op_sub->find_entry ("RULE");
          if (op_value)
            ok = property->getTitleRule ().parse_ed_s (op_value->get_value ());
        }

      if (ok)
        _title_prop_list.push_back(property);
      else
        delete property;
    }
}

//! @brief
void
AutoProperties::parseDecorProperty(CfgParser::Entry *op_section)
{
  op_section = op_section->get_section ();

  bool ok;
  DecorProperty *property;
  CfgParser::Entry *op_sub;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      op_sub = op_section->get_section ();

      ok = false;
      property = new DecorProperty ();

      if (parseProperty (op_sub, property))
        {
          CfgParser::Entry *op_value = op_sub->find_entry ("DECOR");
          if (op_value)
            {
              property->applyAdd (APPLY_ON_START);
              property->setName (op_value->get_value ());
            }
        }

      if (ok)
        _decor_prop_list.push_back(property);
      else
        delete property;
    }
}

#ifdef HARBOUR
//! @brief
void
AutoProperties::parseDockAppProperty(CfgParser::Entry *op_section)
{
  op_section = op_section->get_section ();

  // Reset harbour sort, set to true if POSITION property found.
  _harbour_sort = false;

  bool ok;
  DockAppProperty *property;
  CfgParser::Entry *op_sub;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      op_sub = op_section->get_section ();

      ok = false;
      property = new DockAppProperty ();

      if (parseProperty (op_sub, property))
        {
          CfgParser::Entry *op_value = op_sub->find_entry ("POSITION");
          if (op_value)
            {
              _harbour_sort = true;
              ok = true;

              int position = strtol (op_value->get_value ().c_str(), NULL, 10);
              property->setPosition (position);
            }
        }

      if (ok)
        _dock_app_prop_list.push_back(property);
      else
        delete property;
    }
}
#endif // HARBOUR

//! @brief Searches the _prop_list for a property
AutoProperty*
AutoProperties::findAutoProperty(const ClassHint* class_hint,
																 int ws, uint type)
{
	AutoProperty *property =
		(AutoProperty*) findProperty(class_hint, &_prop_list, ws, type);
	if (property)
		return (AutoProperty*) property;
	return NULL;
}

//! @brief Searches the _title_prop_list for a property
TitleProperty*
AutoProperties::findTitleProperty(const ClassHint* class_hint,
																	uint type)
{
	TitleProperty *property =
		(TitleProperty*) findProperty(class_hint, &_title_prop_list, -1, type);

	return property;
}

//! @brief
DecorProperty*
AutoProperties::findDecorProperty(const ClassHint* class_hint)
{
	DecorProperty *property =
		(DecorProperty*) findProperty(class_hint, &_decor_prop_list, -1, 0);

	return property;
}

#ifdef HARBOUR
DockAppProperty*
AutoProperties::findDockAppProperty(const ClassHint *class_hint)
{
	DockAppProperty *property =
		(DockAppProperty*) findProperty(class_hint, &_dock_app_prop_list, -1, 0);

	return property;
}
#endif // HARBOUR

//! @brief Removes all ApplyOnStart actions as they consume memory
void
AutoProperties::removeApplyOnStart(void)
{
	list<Property*>::iterator it(_prop_list.begin());
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

//! @brief Tries to match a class hint against an autoproperty data entry
bool
AutoProperties::matchAutoClass(const ClassHint &hint, Property *prop)
{
  bool ok = false;

  if ((prop->getHintName() == hint.h_name)
       && (prop->getHintClass() == hint.h_class))
    {
      ok = true;
      if (prop->getTitle ().is_match_ok ())
        ok = (prop->getTitle () == hint.title);
      if (ok && prop->getRole ().is_match_ok ())
        ok = (prop->getRole () == hint.h_role);
    }

  return ok;
}
