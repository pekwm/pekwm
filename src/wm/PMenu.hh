//
// PMenu.hh for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PMENU_HH_
#define _PEKWM_PMENU_HH_

#include "config.h"

#include <map>
#include <string>

#include "pekwm.hh"
#include "AutoProperties.hh"
#include "CfgParser.hh"
#include "PDecor.hh"
#include "PWinObjReference.hh"

#include "tk/PPixmapSurface.hh"

class PTexture;
class ActionEvent;
class Theme;

class PMenu : public PDecor {
public:
	class Item : public PWinObjReference {
	public:
		enum Type {
			MENU_ITEM_NORMAL,
			MENU_ITEM_SEPARATOR,
			MENU_ITEM_HIDDEN
		};
		Item(const std::string& name, PWinObj* wo_ref = nullptr,
		     PTexture* icon = nullptr);
		virtual ~Item(void);

		inline int getX(void) const { return _gm.x; }
		inline int getY(void) const { return _gm.y; }
		inline int getWidth(void) const { return _gm.width; }
		inline int getHeight(void) const { return _gm.height; }
		inline const std::string &getName(void) const { return _name; }
		inline const ActionEvent &getAE(void) const { return _ae; }
		inline PTexture *getIcon(void) { return _icon; }
		inline PMenu::Item::Type getType(void) const { return _type; }

		inline void setX(int x) { _gm.x = x; }
		inline void setY(int y) { _gm.y = y; }
		inline void setWidth(int width) { _gm.width = width; }
		inline void setHeight(int height) { _gm.height = height; }
		inline void setName(const std::string &name) { _name = name; }
		inline void setAE(const ActionEvent &ae) { _ae = ae; }
		inline void setType(PMenu::Item::Type type) { _type = type; }

		inline void setCreator(PMenu::Item *c) { _creator = c; }
		inline PMenu::Item *getCreator(void) const { return _creator; }

	private:
		Geometry _gm;
		std::string _name;

		ActionEvent _ae; // used for specifying action of the entry

		PMenu::Item::Type _type; // normal, separator or hidden item
		PTexture *_icon; // icon texture.

		PMenu::Item *_creator; /**< pointer to the dynamic action item
					  that created this item. */
	};

	typedef std::vector<Item*> item_vec;
	typedef item_vec::iterator item_it;
	typedef item_vec::const_iterator item_cit;

	PMenu(const std::string &title,
	      const std::string &name, const std::string& decor_name = "MENU",
	      bool init = true);
	virtual ~PMenu(void);

	// START - PWinObj interface.
	virtual void unmapWindow(void);

	virtual void setFocused(bool focused);
	virtual void setFocusable(bool focusable);

	virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
	virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
	virtual ActionEvent *handleExposeEvent(XExposeEvent*);
	virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
	virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
	virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);
	// END - PWinObj interface.

	// START - PDecor interface.
	virtual void loadTheme(void);
	// END - PDecor interface.

	static inline PMenu *findMenu(Window win) {
		std::map<Window, PMenu*>::iterator it = _menu_map.find(win);
		return (it != _menu_map.end()) ? it->second : 0;
	}

	const std::string &getName(void) { return _name; }
	PMenu::Item *getItemCurr(void) {
		return _item_curr < _items.size()
			? _items[_item_curr] : nullptr;
	}
	void selectNextItem(void);
	void selectPrevItem(void);

	// modifying menu content
	void setTitle(const std::string &title);
	void setMenuWidth(uint width) { _menu_width = width; }

	virtual void insert(PMenu::Item *item);
	virtual void insert(std::vector<PMenu::Item*>::iterator at,
			    PMenu::Item *item);
	virtual void insert(const std::string &name, PWinObj *wo_ref = 0,
			    PTexture *icon = 0);
	virtual void insert(const std::string &name, const ActionEvent &ae,
			    PWinObj *wo_ref = 0, PTexture *icon = 0);
	virtual void remove(PMenu::Item *item);
	virtual void removeAll(void);

	virtual void reload(CfgParser::Entry*) { }
	void buildMenu(void);

	inline uint size(void) const { return _items.size(); }
	item_it m_begin_non_const(void) { return _items.begin(); }
	item_cit m_begin(void) { return _items.begin(); }
	item_cit m_end(void) { return _items.end(); }

	inline MenuType getMenuType(void) const { return _menu_type; }

	virtual void handleItemExec(PMenu::Item*) { }

	// control ( mapping, unmapping etc )
	void mapUnderMouse(void);
	void mapSubmenu(PMenu *menu, bool focus = false);
	void unmapSubmenus(void);
	void unmapAll(void);
	void gotoParentMenu(void);

	void select(PMenu::Item *item, bool unmap_submenu = true);
	void selectItem(std::vector<PMenu::Item*>::const_iterator item,
			bool unmap_submenu = true);
	void deselectItem(bool unmap_submenu = true);
	bool selectItemNum(uint num);
	void selectItemRel(int off);
	void exec(PMenu::Item *item);

protected:
	void checkItemWORef(PMenu::Item *item);

private:
	PMenu(const PMenu&);
	PMenu& operator=(const PMenu&);

	void renderSelectedItem(void);

	void handleItemEvent(MouseEventType type, int x, int y);

	void buildMenuCalculate(void);
	void buildMenuCalculateMaxWidth(uint &width,
					uint &icon_width, uint &icon_height);
	void buildMenuCalculateColumns(uint &width, uint &height);
	void buildMenuPlace(void);
	void buildMenuRender(void);
	void buildMenuRenderState(PSurface *surf, ObjectState state);
	void buildMenuRenderItem(PSurface *surf, ObjectState state,
				 PMenu::Item* item);
	void buildMenuRenderItemNormal(PSurface *surf, ObjectState state,
				       PMenu::Item* item);
	void buildMenuRenderItemSeparator(PSurface *surf, ObjectState state,
					  PMenu::Item* item);

	PMenu::Item *findItem(int x, int y);
	void makeInsideScreen(int x, int y);

	void applyTitleRules(const std::string &title);

protected:
	std::string _name; //!< Name of menu, must be unique
	MenuType _menu_type; //!< Type of menu
	PMenu *_menu_parent;

	ClassHint _class_hint; /**< Class information for menu. */

private:
	// menu content data
	item_vec _items;
	item_vec::size_type _item_curr;

	PWinObj *_menu_wo;
	PDecor::TitleItem _title;

	// menu render data
	PPixmapSurface _menu_bg_fo;
	PPixmapSurface _menu_bg_un;
	PPixmapSurface _menu_bg_se;

	// menu disp data
	uint _menu_width; /**< Static set menu width. */
	uint _item_height;
	uint _item_width_max;
	uint _item_pad_horz;
	uint _icon_width;
	uint _icon_height;
	uint _separator_height;

	uint _size; // size, hidden items excluded
	uint _rows, _cols;
	bool _scroll;
	uint _has_submenu;

	static std::map<Window, PMenu*> _menu_map;
};

#endif // _PEKWM_PMENU_HH_
