//
// keys.hh for pekwm
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

#ifndef _KEYS_HH_
#define _KEYS_HH_

#include "config.hh"
#include "screeninfo.hh"

#include <string>
#include <vector>

#include <X11/Xlib.h>

class Keys
{
	class KeyAction : public Action {
	public:
		KeyAction() : key(0), mod(0) { }
		~KeyAction() { }

	public:
		KeyCode key;
		unsigned int mod;
	};

public:
	Keys(Config *c, ScreenInfo *s);
	~Keys();

	void loadKeys(void);
	void grabKeys(Window w);
	void ungrabKeys(Window w);

	Action* getActionFromKeyEvent(XKeyEvent *ev);

private:
	Config *cfg;
	ScreenInfo *scr;

	std::vector<KeyAction> m_keygrabs;
	std::vector<KeyAction>::iterator m_it;

	int getMod(const std::string &mod);

	unsigned int num_lock;
	unsigned int scroll_lock;

	struct modlist_item {
		const char *name;
		int mask;

		inline bool operator == (std::string s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static modlist_item m_modlist[];
};

#endif // _KEYS_HH_

#endif // KEYS
