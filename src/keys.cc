//
// keys.cc for pekwm
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

#ifdef KEYS

#include "keys.hh"
#include "baseconfig.hh"
#include "windowmanager.hh"
#include "util.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

Keys::Keys(Config *c, ScreenInfo *s) :
cfg(c), scr(s),
num_lock(0), scroll_lock(0)
{
	num_lock = scr->getNumLock();
	scroll_lock = scr->getScrollLock();

	loadKeys();
}

Keys::~Keys()
{
	if (m_keygrabs.size())
			m_keygrabs.clear();
}

//! @fn    void loadKeys(void)
//! @brief Parses the "KeyFile" and inserts into m_keygrabs.
//! NOTE: if m_keygrabs holds any keygrabs they will be flushed before
//! reloading the new keybindings.
void
Keys::loadKeys(void)
{
	BaseConfig key_cfg;

	if (!key_cfg.load(cfg->getKeyFile())) {
		string key_file = DATADIR "/keys";
		if (!key_cfg.load(key_file)) {
			cerr << "Can't load the keyfile: " << key_file << endl;
			return;
		}
	}

	if (m_keygrabs.size())
		m_keygrabs.clear();

	BaseConfig::CfgSection *cs;
	string s_value;
	vector<string> keys;
	vector<string>::iterator it;

	KeyAction key; // move out from the loop

	while ((cs = key_cfg.getNextSection())) {
		key.action = cfg->getAction(cs->getName(), KEYGRABBER_OK);
		if (key.action == NO_ACTION)
			continue; // invalid action

		if (!cs->getValue("KEY", s_value))
			continue; // we need a key
		key.key =
			XKeysymToKeycode(scr->getDisplay(), XStringToKeysym(s_value.c_str()));
		if (key.key == NoSymbol)
			continue; // we need key

		key.mod = 0;
		if (cs->getValue("MOD", s_value)) { // we want a modifier?
			keys.clear();
			if (Util::splitString(s_value, keys, " \t")) {
				for (it = keys.begin(); it != keys.end(); ++it) {
					key.mod |= cfg->getMod(*it);
				}
			}
		}

		// check if we need a param
		if (cs->getValue("PARAM", s_value)) {
			switch (key.action) {
			case EXEC:
			case RESTART_OTHER:
				key.s_param = s_value;
					break;
			case SEND_TO_WORKSPACE:
			case GO_TO_WORKSPACE:
				key.i_param = atoi(s_value.c_str()) - 1;
				if (key.i_param < 0)
					key.i_param = 0;
				break;
			case NUDGE_HORIZONTAL:
			case NUDGE_VERTICAL:
			case RESIZE_HORIZONTAL:
			case RESIZE_VERTICAL:
				key.i_param = atoi(s_value.c_str());
				break;
			case MOVE_TO_CORNER:
				key.i_param = cfg->getCorner(s_value);
				break;
			case ACTIVATE_CLIENT_NUM:
				key.i_param = atoi(s_value.c_str()) - 1;
				break;
			default:
				// do nothing
				break;
			}
		}

		m_keygrabs.push_back(key);
	}
}

//! @fn    void grabKeys(Window window)
//! @brief Grabs all the keybindings in m_keybindings on the Window w
//! @param w Window to grab the keys on
void
Keys::grabKeys(Window window)
{
	Display *dpy = scr->getDisplay(); // convinience

	for (m_it = m_keygrabs.begin(); m_it != m_keygrabs.end(); ++m_it) {
		XGrabKey(dpy, m_it->key, m_it->mod,
						 window, true, GrabModeAsync, GrabModeAsync);
		XGrabKey(dpy, m_it->key, m_it->mod|LockMask,
						 window, true, GrabModeAsync, GrabModeAsync);

		if (num_lock) {
			XGrabKey(dpy, m_it->key, m_it->mod|num_lock,
							 window, true, GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, m_it->key, m_it->mod|num_lock|LockMask,
							 window, true, GrabModeAsync, GrabModeAsync);
		}
		if (scroll_lock) {
			XGrabKey(dpy, m_it->key, m_it->mod|scroll_lock,
							 window, true, GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, m_it->key, m_it->mod|scroll_lock|LockMask,
							 window, true, GrabModeAsync, GrabModeAsync);
		}
		if (num_lock && scroll_lock) {
			XGrabKey(dpy, m_it->key, m_it->mod|num_lock|scroll_lock,
							 window, true, GrabModeAsync, GrabModeAsync);
			XGrabKey(dpy, m_it->key, m_it->mod|num_lock|scroll_lock|LockMask,
							 window, true, GrabModeAsync, GrabModeAsync);
		}
	}
}

//! @fn    void ungrabKeys(Window w)
//! @brief UnGrabs all the keybindings on the Window w
//! @param w Window to ungrab keys on
void
Keys::ungrabKeys(Window w)
{
	XUngrabKey(scr->getDisplay(), AnyKey, AnyModifier, w);
}

//! @fn    void getActionFromKeyEvent(XKeyEvent *ev)
//! @brief Tries to match the XKeyEvent to an usefull action and return it
//! @param ev XKeyEvent to match.
Action*
Keys::getActionFromKeyEvent(XKeyEvent *ev)
{
	if (!ev)
		return NULL;

	// remove unused modifiers
	ev->state &= ~num_lock & ~scroll_lock & ~LockMask;

	for (m_it = m_keygrabs.begin(); m_it != m_keygrabs.end(); ++m_it) {
		if (m_it->mod == ev->state && m_it->key == ev->keycode) {
			return &*m_it;
		}
	}

	return NULL;
}

#endif // KEYS
