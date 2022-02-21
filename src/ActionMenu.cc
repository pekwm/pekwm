//
// ActionMenu.cc for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <cstdio>
#include <set>

#include "Charset.hh"
#include "Debug.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "TextureHandler.hh"
#include "ImageHandler.hh"

#include "Config.hh"
#include "ActionHandler.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Util.hh"
#include "WmUtil.hh"

//! @brief ActionMenu constructor
//! @param type Type of menu
//! @param title Title of menu
//! @param name Name of the menu, empty for dynamic else should be unique
//! @param decor_name Name of decor to use, defaults to MENU.
ActionMenu::ActionMenu(MenuType type, ActionHandler *act,
                       const std::string &title, const std::string &name,
                       const std::string &decor_name)
	: WORefMenu(title, name, decor_name),
	  _act(act),
	  _insert_at(0),
	  _has_dynamic(false)
{
	_menu_type = type;

	if (_menu_type == WINDOWMENU_TYPE) {
		_action_ok = WINDOWMENU_OK;
	} else if (_menu_type == ROOTMENU_TYPE
		   || _menu_type == ROOTMENU_STANDALONE_TYPE) {
		_action_ok = ROOTMENU_OK;
	}
}

//! @brief ActionMenu destructor
ActionMenu::~ActionMenu(void)
{
	removeAll();
}

// START - PWinObj interface.

//! @brief Rebuilds the vmenu and if it has any items after it shows it.
void
ActionMenu::mapWindow(void)
{
	// find and rebuild the dynamic entries
	if (! isMapped() && _has_dynamic) {
		uint size_before = size();
		rebuildDynamic();
		if (size_before != size()) {
			buildMenu();
		}
	}

	PMenu::mapWindow();
}

//! @brief Hides and removes all items created by dynamic entries.
void
ActionMenu::unmapWindow(void)
{
	if (! isMapped()) {
		return;
	}

	if (_has_dynamic) {
		removeDynamic();
	}

	PMenu::unmapWindow();
}

// END - PWinObj interface.

//! @brief Handle item exec.
void
ActionMenu::handleItemExec(PMenu::Item *item)
{
	if (! item) {
		return;
	}

	ActionPerformed ap(getWORef(), item->getAE());
	_act->handleAction(&ap);
}


/**
 * Re-reads the configuration clearing old entries from the menu. This
 * adds the ICON_PATH to the ImageHandler before loading to support
 * loading icons properly.
 *
 * @param section CfgParser::Entry with menu configuration
 */
void
ActionMenu::reload(CfgParser::Entry *section)
{
	// Clear menu
	removeAll();

	// Parse section (if any)
	_insert_at = 0;
	{
		WithIconPath with_icon_path(pekwm::config(), pekwm::imageHandler());
		parse(section);
	}

	// Build menu from parsed content
	buildMenu();
}

//! @brief Checks if we have a position set for where to insert.
void
ActionMenu::insert(PMenu::Item *item)
{
	if (! item) {
		return;
	}

	checkItemWORef(item);

	insert(m_begin_non_const() + _insert_at, item);
	++_insert_at;
}

void
ActionMenu::insert(std::vector<PMenu::Item*>::iterator at, PMenu::Item *item)
{
	WORefMenu::insert(at, item);
}

void
ActionMenu::insert(const std::string &name, PWinObj *wo_ref, PTexture *icon)
{
	WORefMenu::insert(name, wo_ref, icon);
}

void
ActionMenu::insert(const std::string &name, const ActionEvent &ae,
                   PWinObj *wo_ref, PTexture *icon)
{
	WORefMenu::insert(name, ae, wo_ref, icon);
}

//! @brief Removes a BaseMenuItem from the menu
void
ActionMenu::remove(PMenu::Item *item)
{
	if (! item) {
		return;
	}

	if (item->getIcon()) {
		pekwm::textureHandler()->returnTexture(item->getIcon());
	}

	if (item->getWORef() && (item->getWORef()->getType() == WO_MENU)) {
		delete item->getWORef();
	}

	PMenu::remove(item);
}

void
ActionMenu::removeAll(void)
{
	while (size()) {
		remove(*m_begin());
	}
}

//! @brief Parse config and push items into menu
//! @param cs Section object to read config from
//! @param menu BaseMenu object to push object in
//! @param has_dynamic If true the menu being parsed is dynamic, defaults to false.
void
ActionMenu::parse(CfgParser::Entry *section, PMenu::Item *parent)
{
	if (! section) {
		return;
	}

	if (section->getValue().size()) {
		const std::string& title(section->getValue());
		setTitle(title);
		_title_base = title;
	}

	CfgParser::Entry *value;
	ActionEvent ae;

	PMenu::Item *item = 0;
	PTexture *icon = 0;

	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		item = 0;

		if (*(*it) == "SUBMENU") {
			CfgParser::Entry *sub_section = (*it)->getSection();
			if (sub_section) {
				icon = getIcon(sub_section->findEntry("ICON"));

				const std::string& title = (*it)->getValue();
				ActionMenu *submenu =
					new ActionMenu(_menu_type, _act, title, title);
				submenu->_menu_parent = this;
				submenu->parse(sub_section, parent);
				submenu->buildMenu();

				const std::string& sub_title = sub_section->getValue();
				item = new PMenu::Item(sub_title, submenu, icon);
				item->setCreator(parent);
			} else {
				USER_WARN("submenu entry does not contain any section");
			}
		} else if (*(*it) == "SEPARATOR") {
			// No icon support on separators.
			item = new PMenu::Item("", 0, 0);
			item->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
			item->setCreator(parent);

		} else {
			CfgParser::Entry *sub_section = (*it)->getSection();
			if (sub_section) {
				// Inside of the Entry = "foo" { ... } section, here
				// Actions and Icon are the valid options.
				value = sub_section->findEntry("ACTIONS");
				if (value && ActionConfig::parseActions(value->getValue(), ae,
									_action_ok)) {
					icon = getIcon(sub_section->findEntry("ICON"));

					const std::string& sub_name = sub_section->getValue();
					item = new PMenu::Item(sub_name, 0, icon);
					item->setCreator(parent);
					item->setAE(ae);

					if (ae.isOnlyAction(ACTION_MENU_DYN)) {
						_has_dynamic = true;
						item->setType(PMenu::Item::MENU_ITEM_HIDDEN);
					}
				}
			}
		}

		// If an item was successfully created, insert it to the menu.
		if (item) {
			insert(item);
		}
	}
}

/**
 * Get icon texture from parser value.
 *
 * @param value Entry to get icon name from.
 * @return PTexture if icon was loaded, else 0.
 */
PTexture*
ActionMenu::getIcon(CfgParser::Entry *value)
{
	// Skip blank icons
	if (! value || ! value->getValue().size()) {
		return 0;
	}

	PTexture *icon = 0;

	// Try to load the icon as a complete texture specification, if
	// that fails load is an scaled image.
	icon = pekwm::textureHandler()->getTexture(value->getValue());
	if (! icon) {
		icon = pekwm::textureHandler()->getTexture("IMAGE " + value->getValue()
							   + "#SCALED");
	}

	return icon;
}

/**
 * Executes all Dynamic entries in the menu.
 */
void
ActionMenu::rebuildDynamic(void)
{
	PWinObj *wo_ref = getWORef();

	// Export environment before to dynamic script.
	Client *client = 0;
	if (wo_ref && wo_ref->getType() == WO_CLIENT) {
		client = static_cast<Client*>(wo_ref);
	}
	Client::setClientEnvironment(client);

	// Setup icon path before parsing.
	WithIconPath with_icon_path(pekwm::config(), pekwm::imageHandler());

	PMenu::Item* item = nullptr;
	std::vector<PMenu::Item*>::const_iterator it = m_begin();
	for (; it != m_end(); ++it) {
		if ((*it)->getAE().isOnlyAction(ACTION_MENU_DYN)) {
			_insert_at = it - m_begin();

			item = *it;

			CfgParser dynamic;
			std::string cmd = (*it)->getAE().action_list.front().getParamS();
			CfgParser::Entry *section = runDynamic(dynamic, cmd);
			if (section != nullptr) {
				_has_dynamic = true;
				parse(section, *it);
			}

			it = find(m_begin(), m_end(), item);
		}
	}
	_insert_at = size();
}

CfgParser::Entry*
ActionMenu::runDynamic(CfgParser& parser, const std::string& src)
{
	if (parser.parse(src, CfgParserSource::SOURCE_COMMAND)) {
		return parser.getEntryRoot()->findSection("DYNAMIC");
	}
	return nullptr;
}

//! @brief Remove all entries from the menu created by dynamic entries.
void
ActionMenu::removeDynamic(void)
{
	std::set<PMenu::Item *> dynlist;

	std::vector<PMenu::Item*>::const_iterator it = m_begin();
	for (; it != m_end(); ++it) {
		if ((*it)->getType() == PMenu::Item::MENU_ITEM_HIDDEN) {
			dynlist.insert(*it);
		}
	}

	std::vector<PMenu::Item*> items_to_remove;
	it = m_begin();
	for (; it != m_end(); ++it) {
		if (dynlist.find((*it)->getCreator()) != dynlist.end()) {
			if ((*it)->getWORef()
			    && ((*it)->getWORef()->getType() == WO_MENU)) {
				delete (*it)->getWORef();
			}
			items_to_remove.push_back(*it);
		}
	}

	it = items_to_remove.begin();
	for (; it != items_to_remove.end(); ++it) {
		remove(*it);
	}
}
