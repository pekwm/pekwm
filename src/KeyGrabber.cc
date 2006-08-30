//
// KeyGrabber.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef KEYS

#include "KeyGrabber.hh"

#include "Config.hh"
#include "ScreenInfo.hh"
#include "BaseConfig.hh"
#include "Util.hh"

#include <iostream>

extern "C" {
#include <X11/Xutil.h>
#include <X11/keysym.h>
}

using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::vector;

//! @fn    Chain(unsigned int mod, unsigned int key)
//! @brief Constructor for Chain class
KeyGrabber::Chain::Chain(unsigned int mod, unsigned int key) :
_mod(mod), _key(key)
{
}

//! @fn    ~Chain()
//! @brief Destructor for Chain class
KeyGrabber::Chain::~Chain()
{
	unload();
}

//! @fn    void unload(void)
//! @brief Unloads all chains and keys
void
KeyGrabber::Chain::unload(void)
{
	list<KeyGrabber::Chain*>::iterator it = _chains.begin();
	for (; it != _chains.end(); ++it)
		delete *it;
	_chains.clear();
	_keys.clear();
}

//! @fn    Chain* findChain(XKeyEvent* ev)
//! @brief Searches the _chains list for an action
KeyGrabber::Chain*
KeyGrabber::Chain::findChain(XKeyEvent* ev)
{
	list<KeyGrabber::Chain*>::iterator it = _chains.begin();

	for (; it != _chains.end(); ++it) {
		if (((*it)->getMod() == ev->state) && ((*it)->getKey() == ev->keycode))
			return *it;
	}

	return NULL;
}

//! @fn    Action* findAction(XKeyEvent* ev)
//! @brief Searches the _keys list for an action
ActionEvent*
KeyGrabber::Chain::findAction(XKeyEvent* ev)
{
	list<ActionEvent>::iterator it = _keys.begin();

	for (; it != _keys.end(); ++it) {
		if ((it->mod == ev->state) && (it->sym == ev->keycode))
			return &*it;
	}

	return NULL;
}

//! @fn    KeyGrabber(ScreenInfo *scr, Config *cfg)
//! @brief Constructor for KeyGrabber class
KeyGrabber::KeyGrabber(ScreenInfo *scr, Config *cfg) :
_scr(scr), _cfg(cfg),
#ifdef MENUS
_menu_chain(0, 0),
#endif // MENUS
_global_chain(0, 0), _moveresize_chain(0, 0)
{
	_num_lock = _scr->getNumLock();
	_scroll_lock = _scr->getScrollLock();
}

//! @fn    ~KeyGrabber()
//! @brief Destructor for KeyGrabber class
KeyGrabber::~KeyGrabber()
{
}

//! @fn    void load(const std::string &file)
//! @brief Parses the "KeyFile" and inserts into _global_keys.
//! NOTE: if _global_keys holds any keygrabs they will be flushed before
//! reloading the new keybindings.
void
KeyGrabber::load(const std::string &file)
{
	BaseConfig key_cfg;

	if (!key_cfg.load(file)) {
		if (!key_cfg.load(SYSCONFDIR "/keys")) {
			cerr << "Can't load the keyfile: " << file << endl;
			return;
		}
	}

	BaseConfig::CfgSection *sect;

	if ((sect = key_cfg.getSection("GLOBAL"))) {
		_global_chain.unload();
		parseGlobalChain(sect, &_global_chain);
	}

	if ((sect = key_cfg.getSection("MOVERESIZE"))) {
		_moveresize_chain.unload();
		parseMoveResizeChain(sect, &_moveresize_chain);
	}

#ifdef MENUS
	if ((sect = key_cfg.getSection("MENU"))) {
		_menu_chain.unload();
		parseMenuChain(sect, &_menu_chain);
	}
#endif // MENUS
}

//! @fn    void parseGlobalChain(BaseConfig::CfgSection* sect, KeyGrabber::Chain* chain)
//! @brief
void
KeyGrabber::parseGlobalChain(BaseConfig::CfgSection* sect,
														 KeyGrabber::Chain* chain)
{
	BaseConfig::CfgSection *sub_sect;

	string name;
	ActionEvent ae;
	unsigned int key, mod;

	while ((sub_sect = sect->getNextSection())) {
		if (*sub_sect == "CHAIN") {
			// figure out mod and key, create a new chain
			if (sub_sect->getValue("NAME", name)) {
				if (_cfg->parseKey(name, mod, key)) {
					KeyGrabber::Chain *sub_chain = new Chain(mod, key);
					parseGlobalChain(sub_sect, sub_chain);
					chain->addChain(sub_chain);
				}
			} 
		} else if (_cfg->parseActionEvent(sub_sect, ae, KEYGRABBER_OK, false))
			chain->addAction(ae);
	}
}

//! @fn    void parseMoveResizeChain(BaseConfig::CfgSection* sect, KeyGrabber::Chain* chain)
//! @brief
void
KeyGrabber::parseMoveResizeChain(BaseConfig::CfgSection* sect,
																 KeyGrabber::Chain* chain)
{
	BaseConfig::CfgSection *sub_sect;

	string value;
	ActionEvent ae;
	unsigned int key, mod;

	while ((sub_sect = sect->getNextSection())) {
		if (*sub_sect == "CHAIN") {
			// figure out mod and key, create a new chain
			if (sub_sect->getValue("NAME", value)) {
				if (_cfg->parseKey(value, mod, key)) {
					KeyGrabber::Chain *sub_chain = new Chain(mod, key);
					parseMoveResizeChain(sub_sect, sub_chain);
					chain->addChain(sub_chain);
				}
			}
		} else if (_cfg->parseMoveResizeEvent(sub_sect, ae))
			chain->addAction(ae);
	}
}

#ifdef MENUS
//! @fn    void parseMenuChain(BaseConfig::CfgSection* sect, KeyGrabber::Chain* chain)
//! @brief
void
KeyGrabber::parseMenuChain(BaseConfig::CfgSection* sect,
													 KeyGrabber::Chain* chain)
{
	BaseConfig::CfgSection *sub_sect;

	string value;
	ActionEvent ae;
	unsigned int key, mod;

	while ((sub_sect = sect->getNextSection())) {
		if (*sub_sect == "CHAIN") {
			// figure out mod and key, create a new chain
			if (sub_sect->getValue("NAME", value)) {
				if (_cfg->parseKey(value, mod, key)) {
					KeyGrabber::Chain *sub_chain = new Chain(mod, key);
					parseGlobalChain(sub_sect, sub_chain);
					chain->addChain(sub_chain);
				} 
			}
		} else if (_cfg->parseMenuEvent(sub_sect, ae))
			chain->addAction(ae);
	}
}
#endif // MENUS

//! @fn    void grabKeys(Window window)
//! @brief Grabs all the keybindings in _keybindings on the Window w
//! @param w Window to grab the keys on
void
KeyGrabber::grabKeys(Window window)
{
	const list<KeyGrabber::Chain*> chains = _global_chain.getChainList();
	list<KeyGrabber::Chain*>::const_iterator c_it = chains.begin();
	for (; c_it != chains.end(); ++c_it)
		grabKey(window, (*c_it)->getMod(), (*c_it)->getKey());

	const list<ActionEvent> keys = _global_chain.getKeyList();
	list<ActionEvent>::const_iterator k_it = keys.begin();
	for (; k_it != keys.end(); ++k_it)
		grabKey(window, k_it->mod, k_it->sym);
}

//! @fn    void grabKey(Window window, unsigned int mod, unsigned int key)
//! @brief Grabs a key on a window with "all possible" modifiers.
void
KeyGrabber::grabKey(Window window, unsigned int mod, unsigned int key)
{
	Display *dpy = _scr->getDisplay(); // convenience

	XGrabKey(dpy, key, mod, window, true, GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, key, mod|LockMask, window, true, GrabModeAsync, GrabModeAsync);

	if (_num_lock) {
		XGrabKey(dpy, key, mod|_num_lock,
						 window, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_num_lock|LockMask,
						 window, true, GrabModeAsync, GrabModeAsync);
	}
	if (_scroll_lock) {
		XGrabKey(dpy, key, mod|_scroll_lock,
						 window, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_scroll_lock|LockMask,
						 window, true, GrabModeAsync, GrabModeAsync);
	}
	if (_num_lock && _scroll_lock) {
		XGrabKey(dpy, key, mod|_num_lock|_scroll_lock,
						 window, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_num_lock|_scroll_lock|LockMask,
						 window, true, GrabModeAsync, GrabModeAsync);
	}
}

//! @fn    void ungrabKeys(Window w)
//! @brief UnGrabs all the keybindings on the Window w
//! @param w Window to ungrab keys on
void
KeyGrabber::ungrabKeys(Window w)
{
	XUngrabKey(_scr->getDisplay(), AnyKey, AnyModifier, w);
}

//! @fn    ActionEvent* findAction(XKeyEvent *ev, Chain* chain)
//! @brief Tries to match the XKeyEvent to an usefull action and return it
//! @param ev XKeyEvent to match.
ActionEvent*
KeyGrabber::findAction(XKeyEvent* ev, Chain* chain)
{
	if (!ev)
		return NULL;
	ev->state &= ~_num_lock & ~_scroll_lock & ~LockMask;

	ActionEvent *action = NULL;
	KeyGrabber::Chain *sub_chain = _global_chain.findChain(ev);
	if (sub_chain && _scr->grabKeyboard(_scr->getRoot())) {
		XEvent ev;
		KeyGrabber::Chain *last_chain;
		bool exit = false;

		while (!exit) {
			XMaskEvent(_scr->getDisplay(), KeyPressMask, &ev);
			ev.xkey.state &= ~_num_lock & ~_scroll_lock & ~LockMask;

			if (IsModifierKey(XKeycodeToKeysym(_scr->getDisplay(),
																				 ev.xkey.keycode, 0))) {
				// do nothing
			} else  if ((last_chain = sub_chain->findChain(&ev.xkey))) {
				sub_chain = last_chain;
			} else {
				action = sub_chain->findAction(&ev.xkey);
				exit = true;
			}
		}

		_scr->ungrabKeyboard();
	} else {
		action = chain->findAction(ev);
	}
	
	return action;
}

//! @fn    ActionEvent* findGlobalAction(XKeyEvent* ev)
//! @brief Searches the _global_chain for actions.
ActionEvent*
KeyGrabber::findGlobalAction(XKeyEvent* ev)
{
	return findAction(ev, &_global_chain);
}

//! @fn    ActionEvent* findMoveResizeAction(XKeyEvent *ev)
//! @brief Searches the _moveresize_chain for actions.
ActionEvent*
KeyGrabber::findMoveResizeAction(XKeyEvent *ev)
{
	return findAction(ev, &_moveresize_chain);
}

#ifdef MENUS
//! @fn    ActionEvent* findMenuAction(XKeyEvent *ev)
//! @brief Searches the _menu_chain for actions.
ActionEvent*
KeyGrabber::findMenuAction(XKeyEvent *ev)
{
	return findAction(ev, &_menu_chain);
}
#endif // MENUS

#endif // KEYS
