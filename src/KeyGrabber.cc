//
// KeyGrabber.cc for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "KeyGrabber.hh"

#include "Config.hh"
#include "PScreen.hh"
#include "CfgParser.hh"
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
using std::find;

KeyGrabber* KeyGrabber::_instance = NULL;

//! @brief Constructor for Chain class
KeyGrabber::Chain::Chain(uint mod, uint key) :
_mod(mod), _key(key)
{
}

//! @brief Destructor for Chain class
KeyGrabber::Chain::~Chain(void)
{
	unload();
}

//! @brief Unloads all chains and keys
void
KeyGrabber::Chain::unload(void)
{
	list<KeyGrabber::Chain*>::iterator it(_chains.begin());
	for (; it != _chains.end(); ++it) {
		delete *it;
	}
	_chains.clear();
	_keys.clear();
}

//! @brief Searches the _chains list for an action
KeyGrabber::Chain*
KeyGrabber::Chain::findChain(XKeyEvent *ev)
{
	list<KeyGrabber::Chain*>::iterator it(_chains.begin());

	for (; it != _chains.end(); ++it) {
		if ((((*it)->getMod() == MOD_ANY) || ((*it)->getMod() == ev->state)) &&
				(((*it)->getKey() == 0) || ((*it)->getKey() == ev->keycode))) {
			return *it;
		}
	}

	return NULL;
}

//! @brief Searches the _keys list for an action
ActionEvent*
KeyGrabber::Chain::findAction(XKeyEvent *ev)
{
	list<ActionEvent>::iterator it(_keys.begin());

	for (; it != _keys.end(); ++it) {
		if (((it->mod == MOD_ANY) || (it->mod == ev->state)) &&
				((it->sym == 0) || (it->sym == ev->keycode))) {
			return &*it;
		}
	}

	return NULL;
}

//! @brief KeyGrabber constructor
KeyGrabber::KeyGrabber(PScreen *scr) :
_scr(scr),
#ifdef MENUS
_menu_chain(0, 0),
#endif // MENUS
_global_chain(0, 0), _moveresize_chain(0, 0),
_cmd_d_chain(0, 0)
{
#ifdef DEBUG
	if (_instance != NULL) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "KeyGrabber(" << this << ")::KeyGrabber(" << scr << ")" << endl
				 << " *** _instance allready set: " << _instance << endl;
	}
#endif // DEBUG
	_instance = this;

	_num_lock = _scr->getNumLock();
	_scroll_lock = _scr->getScrollLock();
}

//! @brief Destructor for KeyGrabber class
KeyGrabber::~KeyGrabber(void)
{
	_instance = NULL;
}

//! @brief Parses the "KeyFile" and inserts into _global_keys.
//! If _global_keys holds any keygrabs they will be flushed before
//! reloading the new keybindings.
void
KeyGrabber::load(const std::string &file)
{
  CfgParser key_cfg;

  if (!key_cfg.parse (file, CfgParserSource::SOURCE_FILE))
    {
      if (!key_cfg.parse (SYSCONFDIR "/keys", CfgParserSource::SOURCE_FILE))
        {
          cerr << __FILE__ << "@" << __LINE__ 
               << "Error: no keyfile at " << file << " or "
               << SYSCONFDIR << "/keys" << endl;
          return;
        }
    }

  CfgParser::Entry *op_section;

  op_section = key_cfg.get_entry_root ()->find_section ("GLOBAL");
  if (op_section)
    {
      _global_chain.unload ();
      parseGlobalChain (op_section, &_global_chain);
    }

  op_section = key_cfg.get_entry_root ()->find_section ("MOVERESIZE");
  if (op_section)
    {
      _moveresize_chain.unload ();
      parseMoveResizeChain (op_section, &_moveresize_chain);
    }

  op_section = key_cfg.get_entry_root ()->find_section ("CMDDIALOG");
  if (op_section)
    {
      _cmd_d_chain.unload ();
      parseCmdDialogChain (op_section, &_cmd_d_chain);
    }

#ifdef MENUS
  op_section = key_cfg.get_entry_root ()->find_section ("MENU");
  if (op_section)
    {
      _menu_chain.unload ();
      parseMenuChain (op_section, &_menu_chain);
    }
#endif // MENUS
}

//! @brief Parses chain, getting actions as plain ActionEvents
void
KeyGrabber::parseGlobalChain(CfgParser::Entry *op_section,
                             KeyGrabber::Chain *chain)
{
  op_section = op_section->get_section ();

  ActionEvent ae;
  uint key, mod;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      if (*op_section == "CHAIN")
        {
          // Figure out mod and key, create a new chain.
          if (Config::instance ()->parseKey (op_section->get_value (),
                                             mod, key))
            {
              KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain (mod, key);
              parseGlobalChain (op_section, sub_chain);
              chain->addChain (sub_chain);
            }
        }
      else if (Config::instance ()->parseActionEvent(op_section, ae,
                                                     KEYGRABBER_OK, false))
        {
          chain->addAction(ae);
        }
    }
}

//! @brief Parses chain, getting actions as MoveResizeEvents
void
KeyGrabber::parseMoveResizeChain(CfgParser::Entry *op_section,
                                 KeyGrabber::Chain *chain)
{
  op_section = op_section->get_section ();

  ActionEvent ae;
  uint key, mod;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      if (*op_section == "CHAIN")
        {
          // Figure out mod and key, create a new chain.
          if (Config::instance ()->parseKey(op_section->get_value (), mod, key))
            {
              KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain(mod, key);
              parseMoveResizeChain (op_section, sub_chain);
              chain->addChain (sub_chain);
            }
        }
      else if (Config::instance ()->parseMoveResizeEvent (op_section, ae))
        {
          chain->addAction (ae);
        }
    }
}

//! @brief Parses chain, getting actions as CmdDialog Events
void
KeyGrabber::parseCmdDialogChain(CfgParser::Entry *op_section,
                                KeyGrabber::Chain *chain)
{
  op_section = op_section->get_section ();

  ActionEvent ae;
  uint key, mod;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      if (*op_section == "CHAIN")
        {
          // Figure out mod and key, create a new chain.
          if (Config::instance ()->parseKey (op_section->get_value (),
                                             mod, key))
            {
              KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain(mod, key);
              parseCmdDialogChain (op_section, sub_chain);
              chain->addChain (sub_chain);
            }
        }
      else if (Config::instance()->parseCmdDialogEvent (op_section, ae))
        {
          chain->addAction (ae);
        }
    }
}

#ifdef MENUS
//! @brief Parses chain, getting actions as MenuEvents
void
KeyGrabber::parseMenuChain(CfgParser::Entry *op_section,
                           KeyGrabber::Chain *chain)
{
  op_section = op_section->get_section ();

  ActionEvent ae;
  uint key, mod;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      if (*op_section == "CHAIN")
        {
          // Figure out mod and key, create a new chain.
          if (Config::instance ()->parseKey(op_section->get_value (), mod, key))
            {
              KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain (mod, key);
              parseGlobalChain (op_section, sub_chain);
              chain->addChain (sub_chain);
            }
        }
      else if (Config::instance ()->parseMenuEvent (op_section, ae))
        {
          chain->addAction (ae);
        }
    }
}
#endif // MENUS

//! @brief Grabs all the keybindings in _keybindings on the Window win.
//! @param win Window to grab the keys on.
void
KeyGrabber::grabKeys(Window win)
{
	const list<KeyGrabber::Chain*> chains = _global_chain.getChainList();
	list<KeyGrabber::Chain*>::const_iterator c_it = chains.begin();
	for (; c_it != chains.end(); ++c_it)
		grabKey(win, (*c_it)->getMod(), (*c_it)->getKey());

	const list<ActionEvent> keys = _global_chain.getKeyList();
	list<ActionEvent>::const_iterator k_it = keys.begin();
	for (; k_it != keys.end(); ++k_it)
		grabKey(win, k_it->mod, k_it->sym);
}

//! @brief Grabs a key with state on Window win with "all possible" modifiers.
//! @param win Window to grab the key on.
//! @param mod Modifier state to grab.
//! @param key Key state to grab.
void
KeyGrabber::grabKey(Window win, uint mod, uint key)
{
	Display *dpy = _scr->getDpy(); // convenience

	XGrabKey(dpy, key, mod, win, true, GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, key, mod|LockMask, win, true, GrabModeAsync, GrabModeAsync);

	if (_num_lock) {
		XGrabKey(dpy, key, mod|_num_lock,
						 win, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_num_lock|LockMask,
						 win, true, GrabModeAsync, GrabModeAsync);
	}
	if (_scroll_lock) {
		XGrabKey(dpy, key, mod|_scroll_lock,
						 win, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_scroll_lock|LockMask,
						 win, true, GrabModeAsync, GrabModeAsync);
	}
	if (_num_lock && _scroll_lock) {
		XGrabKey(dpy, key, mod|_num_lock|_scroll_lock,
						 win, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, key, mod|_num_lock|_scroll_lock|LockMask,
						 win, true, GrabModeAsync, GrabModeAsync);
	}
}

//! @brief Ungrabs all the keybindings on the Window win.
//! @param win Window to ungrab keys on.
void
KeyGrabber::ungrabKeys(Window win)
{
	XUngrabKey(_scr->getDpy(), AnyKey, AnyModifier, win);
}

//! @brief Tries to match the XKeyEvent to an usefull action and return it
//! @param ev XKeyEvent to match.
ActionEvent*
KeyGrabber::findAction(XKeyEvent *ev, KeyGrabber::Chain *chain)
{
	if (ev == NULL) {
		return NULL;
	}
	ev->state &= ~_num_lock & ~_scroll_lock & ~LockMask;

	ActionEvent *action = NULL;
	KeyGrabber::Chain *sub_chain = _global_chain.findChain(ev);
	if (sub_chain && _scr->grabKeyboard(_scr->getRoot())) {
		XEvent c_ev;
		KeyGrabber::Chain *last_chain;
		bool exit = false;

		while (!exit) {
			XMaskEvent(_scr->getDpy(), KeyPressMask, &c_ev);
			c_ev.xkey.state &= ~_num_lock & ~_scroll_lock & ~LockMask;

			if (IsModifierKey(XKeycodeToKeysym(_scr->getDpy(),
																				 c_ev.xkey.keycode, 0))) {
				// do nothing
			} else  if ((last_chain = sub_chain->findChain(&c_ev.xkey))) {
				sub_chain = last_chain;
			} else {
				action = sub_chain->findAction(&c_ev.xkey);
				exit = true;
			}
		}

		_scr->ungrabKeyboard();
	} else {
		action = chain->findAction(ev);
	}
	
	return action;
}

//! @brief Finds action matching ev, continues chain if needed
ActionEvent*
KeyGrabber::findAction(XKeyEvent *ev, PWinObj::Type type)
{
	ActionEvent *ae = NULL;

#ifdef MENUS
	if (type == PWinObj::WO_MENU) {
		ae = findAction(ev, &_menu_chain);
	}
#endif // MENUS
	if (type == PWinObj::WO_CMD_DIALOG) {
		ae = findAction(ev, &_cmd_d_chain);
	}

	// no action the menu list, try the global list
	if (ae == NULL) {
		ae = findAction(ev, &_global_chain);
	}

	return ae;
}

//! @brief Searches the _moveresize_chain for actions.
ActionEvent*
KeyGrabber::findMoveResizeAction(XKeyEvent *ev)
{
	return findAction(ev, &_moveresize_chain);
}
