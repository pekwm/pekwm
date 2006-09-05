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

Keys::modlist_item Keys::m_modlist[] = {
	{"Shift", ShiftMask},
	{"Ctrl", ControlMask},
	{"Alt", Mod1Mask},
	//	{"Mod2", Mod2Mask}, // Num Lock
	//	{"Mod3", Mod3Mask}, // Num Lock on Solaris
	{"Mod4", Mod4Mask}, // Meta / Win
	//	{"Mod5", Mod5Mask}, // Scroll Lock
	{"None", 0},
	{"", -1}
};

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
	BaseConfig key_cfg(cfg->getKeyFile(), "*", ";");

	if (!key_cfg.loadConfig()) {
		string key_file = DATADIR "/keys";
		key_cfg.setFile(key_file);

		if (!key_cfg.loadConfig()) {
			cerr << "Can't load the keyfile: " << key_file << endl;
			return;
		}
	}

	if (m_keygrabs.size())
		m_keygrabs.clear();

	vector<string> keys, tmp;
	vector<string>::iterator it;

	string action, value;
	while (key_cfg.getNextValue(action, value)) {
		KeyAction key;

		if ((key.action = cfg->getAction(action, KEYGRABBER_OK)) == NO_ACTION) {
			continue; // not a valid action
		}

		if (tmp.size())
			tmp.clear();
	 	if (keys.size())
			keys.clear();

		bool has_param = false;
		string key_string;
		if ((Util::splitString(value, tmp, ":", 2)) == 2) {
			key_string = tmp.front();
			has_param = true;
		} else {
			key_string = value;
		}

		if (Util::splitString(key_string, keys, " \t")) {
			bool got_mod = false, got_key = false;

			key.mod = 0;
			for (it = keys.begin(); it != keys.end(); ++it) {
				int mod;
				if ((mod = getMod(*it)) != -1) {
					key.mod |= mod;
					got_mod = true;
				} else if (got_mod) {
					key.key =
						XKeysymToKeycode(scr->getDisplay(), XStringToKeysym(it->c_str()));
					if (key.key != NoSymbol) {
						got_key = true;
					}
					break;
				}
			}

			if (got_mod && got_key) {

				// If we have any parameter, check if we use a action that has params
				if (has_param) {
					switch (key.action) {
					case EXEC:
					case RESTART_OTHER:
						key.s_param = tmp[1];
						break;
					case SEND_TO_WORKSPACE:
					case GO_TO_WORKSPACE:
						key.i_param = atoi(tmp[1].c_str()) - 1;
						if (key.i_param < 0)
							key.i_param = 0;
						break;
					case NUDGE_HORIZONTAL:
					case NUDGE_VERTICAL:
					case RESIZE_HORIZONTAL:
					case RESIZE_VERTICAL:
						key.i_param = atoi(tmp[1].c_str());
						break;
					case ACTIVATE_CLIENT_NUM:
						key.i_param = atoi(tmp[1].c_str()) - 1;
						break;
					default:
						// do nothing
						break;
					}
				}

				m_keygrabs.push_back(key);
			}
		}
	}
}

//! @fn    int getMod(const string &mod)
//! @brief Converts the string mod into a usefull X Modifier Mask
//! @param mod String to convert into an X Modifier Mask
//! @return X Modifier Mask if found, else 0
int
Keys::getMod(const string &mod) {
	if (! mod.size())
		return 0;
		
	for (unsigned int i = 0; m_modlist[i].mask != -1; ++i) {
		if (m_modlist[i] ==  mod) {
			return m_modlist[i].mask;
		}
	}
	
	return -1;
}

//! @fn    void grabKeys(Window window)
//! @brief Grabs all the keybindings in m_keybindings on the Window w
//! @param w Window to grab the keys on
void
Keys::grabKeys(Window window)
{
	Display *dpy = scr->getDisplay(); // convinience

	unsigned int i = 0;
	for (m_it = m_keygrabs.begin(); m_it != m_keygrabs.end(); ++m_it, i = 0) {
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
