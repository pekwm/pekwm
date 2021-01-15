//
// KeyGrabber.cc for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "KeyGrabber.hh"

#include "Config.hh"
#include "x11.hh"
#include "CfgParser.hh"
#include "Util.hh"

extern "C" {
#include <X11/Xutil.h>
}

#include <iostream>

//! @brief Constructor for Chain class
KeyGrabber::Chain::Chain(uint mod, uint key)
    : _mod(mod),
      _key(key)
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
    for (auto it : _chains) {
        delete it;
    }
    _chains.clear();
    _keys.clear();
}

//! @brief Searches the _chains list for an action
KeyGrabber::Chain*
KeyGrabber::Chain::findChain(XKeyEvent *ev, bool *matched)
{
    for (auto it : _chains) {
        if (((it->getMod() == MOD_ANY) || (it->getMod() == ev->state)) &&
                ((it->getKey() == 0) || (it->getKey() == ev->keycode))) {
            return it;
        }
    }
    return 0;
}

//! @brief Searches the _keys list for an action
ActionEvent*
KeyGrabber::Chain::findAction(XKeyEvent *ev, bool *matched)
{
    auto it = _keys.begin();
    for (; it != _keys.end(); ++it) {
        if ((it->mod == MOD_ANY || it->mod == ev->state)
            && (it->sym == 0 || it->sym == ev->keycode)) {
            *matched = true;
            return &*it;
        }
    }

    return 0;
}

//! @brief KeyGrabber constructor
KeyGrabber::KeyGrabber(void)
    : _menu_chain(0, 0),
      _global_chain(0, 0), _moveresize_chain(0, 0),
      _input_dialog_chain(0, 0)
{
    _num_lock = X11::getNumLock();
    _scroll_lock = X11::getScrollLock();
}

//! @brief Destructor for KeyGrabber class
KeyGrabber::~KeyGrabber(void)
{
}

//! @brief Parses the "KeyFile" and inserts into _global_keys.
//! If _global_keys holds any keygrabs they will be flushed before
//! reloading the new keybindings.
bool
KeyGrabber::load(const std::string &file, bool force)
{
    if (! force && ! _cfg_files.requireReload(file)) {
        return false;
    }

    CfgParser key_cfg;
    if (! key_cfg.parse(file, CfgParserSource::SOURCE_FILE)) {
        _cfg_files.clear();
        if (! key_cfg.parse(SYSCONFDIR "/keys", CfgParserSource::SOURCE_FILE, true)) {
            std::cerr << __FILE__ << "@" << __LINE__ << "Error: no keyfile at "
                      << file << " or " << SYSCONFDIR "/keys" << std::endl;
            return false;
        }
    }

    if (key_cfg.isDynamicContent()) {
        _cfg_files.clear();
    } else {
        _cfg_files = key_cfg.getCfgFiles();
    }

    CfgParser::Entry *section;

    section = key_cfg.getEntryRoot()->findSection("GLOBAL");
    if (section) {
        _global_chain.unload();
        parseGlobalChain(section, &_global_chain);
    }

    section = key_cfg.getEntryRoot ()->findSection("MOVERESIZE");
    if (section) {
        _moveresize_chain.unload ();
        parseMoveResizeChain (section, &_moveresize_chain);
    }

    // Previously there was only a CmdDialog section, however the text
    // handling parts have been moved into InputDialog but to keep
    // compatibility this check exists.
    section = key_cfg.getEntryRoot()->findSection("INPUTDIALOG");
    if (! section) {
        section = key_cfg.getEntryRoot()->findSection("CMDDIALOG");
    }

    if (section) {
        _input_dialog_chain.unload ();
        parseInputDialogChain (section, &_input_dialog_chain);
    }

    section = key_cfg.getEntryRoot ()->findSection("MENU");
    if (section) {
        _menu_chain.unload();
        parseMenuChain(section, &_menu_chain);
    }

    return true;
}

//! @brief Parses chain, getting actions as plain ActionEvents
void
KeyGrabber::parseGlobalChain(CfgParser::Entry *section, KeyGrabber::Chain *chain)
{
    ActionEvent ae;
    uint key, mod;

    auto it(section->begin());
    for (; it != section->end(); ++it) {
        if ((*it)->getSection() && *(*it) == "CHAIN") {
            // Figure out mod and key, create a new chain.
            if (pekwm::config()->parseKey((*it)->getValue(), mod, key)) {
                KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain(mod, key);
                parseGlobalChain((*it)->getSection(), sub_chain);
                chain->addChain(sub_chain);
            }
        } else if (pekwm::config()->parseActionEvent((*it), ae, KEYGRABBER_OK, false)) {
            chain->addAction(ae);
        }
    }
}

//! @brief Parses chain, getting actions as MoveResizeEvents
void
KeyGrabber::parseMoveResizeChain(CfgParser::Entry *section, KeyGrabber::Chain *chain)
{
    ActionEvent ae;
    uint key, mod;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if ((*it)->getSection() && *(*it) == "CHAIN") {
            // Figure out mod and key, create a new chain.
            if (pekwm::config()->parseKey((*it)->getValue(), mod, key)) {
                KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain(mod, key);
                parseMoveResizeChain((*it)->getSection(), sub_chain);
                chain->addChain(sub_chain);
            }
        } else if (pekwm::config()->parseMoveResizeEvent((*it), ae)) {
            chain->addAction(ae);
        }
    }
}

//! @brief Parses chain, getting actions as InputDialog Events
void
KeyGrabber::parseInputDialogChain(CfgParser::Entry *section, KeyGrabber::Chain *chain)
{
    ActionEvent ae;
    uint key, mod;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if ((*it)->getSection() && *(*it) == "CHAIN") {
            // Figure out mod and key, create a new chain.
            if (pekwm::config()->parseKey((*it)->getValue(), mod, key)) {
                KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain(mod, key);
                parseInputDialogChain((*it)->getSection(), sub_chain);
                chain->addChain(sub_chain);
            }
        } else if (pekwm::config()->parseInputDialogEvent((*it), ae)) {
            chain->addAction(ae);
        }
    }
}

//! @brief Parses chain, getting actions as MenuEvents
void
KeyGrabber::parseMenuChain(CfgParser::Entry *section, KeyGrabber::Chain *chain)
{
    ActionEvent ae;
    uint key, mod;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if ((*it)->getSection() && *(*it) == "CHAIN") {
            // Figure out mod and key, create a new chain.
            if (pekwm::config()->parseKey((*it)->getValue(), mod, key)) {
                KeyGrabber::Chain *sub_chain = new KeyGrabber::Chain (mod, key);
                parseGlobalChain((*it)->getSection(), sub_chain);
                chain->addChain(sub_chain);
            }
        } else if (pekwm::config()->parseMenuEvent((*it), ae)) {
            chain->addAction(ae);
        }
    }
}

//! @brief Grabs all the keybindings in _keybindings on the Window win.
//! @param win Window to grab the keys on.
void
KeyGrabber::grabKeys(Window win)
{
    auto chains = _global_chain.getChains();
    for (auto c_it : chains) {
        grabKey(win, c_it->getMod(), c_it->getKey());
    }

    auto keys = _global_chain.getKeys();
    for (auto k_it : keys) {
        grabKey(win, k_it.mod, k_it.sym);
    }
}

//! @brief Grabs a key with state on Window win with "all possible" modifiers.
//! @param win Window to grab the key on.
//! @param mod Modifier state to grab.
//! @param key Key state to grab.
void
KeyGrabber::grabKey(Window win, uint mod, uint key)
{
    Display *dpy = X11::getDpy(); // convenience

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
    XUngrabKey(X11::getDpy(), AnyKey, AnyModifier, win);
}

//! @brief Tries to match the XKeyEvent to an usefull action and return it
//! @param ev XKeyEvent to match.
ActionEvent*
KeyGrabber::findAction(XKeyEvent *ev, KeyGrabber::Chain *chain, bool *matched)
{
    if (! ev) {
        return 0;
    }

    X11::stripStateModifiers(&ev->state);

    ActionEvent *action = 0;
    KeyGrabber::Chain *sub_chain = _global_chain.findChain(ev, matched);
    if (sub_chain) {
        *matched = true;

        if (X11::grabKeyboard(X11::getRoot())) {
            XEvent c_ev;
            KeyGrabber::Chain *last_chain;
            bool exit = false;

            while (! exit) {
                XMaskEvent(X11::getDpy(), KeyPressMask, &c_ev);
                X11::stripStateModifiers(&c_ev.xkey.state);

                if (IsModifierKey(X11::getKeysymFromKeycode(c_ev.xkey.keycode))) {
                    // do nothing
                } else  if ((last_chain = sub_chain->findChain(&c_ev.xkey, matched))) {
                    sub_chain = last_chain;
                } else {
                    action = sub_chain->findAction(&c_ev.xkey, matched);
                    exit = true;
                }
            }

            X11::ungrabKeyboard();
        }
    } else {
        action = chain->findAction(ev, matched);
    }

    return action;
}

//! @brief Finds action matching ev, continues chain if needed
ActionEvent*
KeyGrabber::findAction(XKeyEvent *ev, PWinObj::Type type, bool *matched)
{
    ActionEvent *ae = 0;

    *matched = false;

    if (type == PWinObj::WO_MENU) {
        ae = findAction(ev, &_menu_chain, matched);
    }
    if (type == PWinObj::WO_CMD_DIALOG || type == PWinObj::WO_SEARCH_DIALOG) {
        ae = findAction(ev, &_input_dialog_chain, matched);
    }

    // no action the menu list, try the global list
    if (! ae) {
        ae = findAction(ev, &_global_chain, matched);
    }

    return ae;
}

//! @brief Searches the _moveresize_chain for actions.
ActionEvent*
KeyGrabber::findMoveResizeAction(XKeyEvent *ev)
{
    bool matched;
    return findAction(ev, &_moveresize_chain, &matched);
}
