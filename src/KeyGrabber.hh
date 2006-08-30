//
// KeyGrabber.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef KEYS

#ifndef _KEYGRABBER_HH_
#define _KEYGRABBER_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "BaseConfig.hh"

class ScreenInfo;
class Config;

#include <string>
#include <list>

extern "C" {
#include <X11/Xlib.h>
}

class KeyGrabber
{
public:
	class Chain
	{
	public:
		Chain(unsigned int mod, unsigned int key);
		~Chain();

		void unload(void);

		inline unsigned int getMod(void) const { return _mod; }
		inline unsigned int getKey(void) const { return _key; }

		inline const std::list<Chain*>& getChainList(void) const { return _chains; }
		inline const std::list<ActionEvent>& getKeyList(void) const { return _keys; }

		inline void addChain(Chain* chain) { _chains.push_back(chain); }
		inline void addAction(const ActionEvent& key) { _keys.push_back(key); }

		Chain* findChain(XKeyEvent* ev);
		ActionEvent* findAction(XKeyEvent* ev);

	private:
		unsigned int _mod, _key;

		std::list<Chain*> _chains;
		std::list<ActionEvent> _keys;
	};

	KeyGrabber(ScreenInfo *scr, Config *cfg);
	~KeyGrabber();

	void load(const std::string &file);
	void grabKeys(Window w);
	void ungrabKeys(Window w);

	ActionEvent* findGlobalAction(XKeyEvent *ev);
	ActionEvent* findMoveResizeAction(XKeyEvent *ev);
#ifdef MENUS
	ActionEvent* findMenuAction(XKeyEvent *ev);
#endif // MENUS

private:
	void grabKey(Window win, unsigned int mod, unsigned int key);

	void parseGlobalChain(BaseConfig::CfgSection* sect, Chain* chain);
	void parseMoveResizeChain(BaseConfig::CfgSection* sect, Chain* chain);
#ifdef MENUS
	void parseMenuChain(BaseConfig::CfgSection* sect, Chain* chain);
#endif // MENUS

	ActionEvent* findAction(XKeyEvent *ev, Chain* chain);

private:
	ScreenInfo *_scr;
	Config *_cfg;

#ifdef MENUS
	Chain _menu_chain;
#endif // MENUS
	Chain _global_chain;
	Chain _moveresize_chain;

	unsigned int _num_lock;
	unsigned int _scroll_lock;
};

#endif // _KEYGRABBER_HH_

#endif // KEYS
