//
// KeyGrabber.hh for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _KEYGRABBER_HH_
#define _KEYGRABBER_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "CfgParser.hh"
#include "PWinObj.hh"

class PScreen;

#include <string>
#include <list>

extern "C" {
#include <X11/Xlib.h>
}

//! @brief Key grabbing and matching routines with key chain support.
class KeyGrabber
{
public:
	//! @brief Key chain state information.
	class Chain
	{
	public:
		Chain(uint mod, uint key);
		~Chain(void);

		void unload(void);

		//! @brief Returns modifier state Chain represents.
		inline uint getMod(void) const { return _mod; }
		//! @brief Returns key state Chain represents.
		inline uint getKey(void) const { return _key; }

		//! @brief Returns list of Chains that follows.
		inline const std::list<Chain*> &getChainList(void) const { return _chains; }
		//! @brief Returns list of Keys in chain.
		inline const std::list<ActionEvent> &getKeyList(void) const { return _keys; }

		//! @brief Adds chain to Chain list.
		inline void addChain(Chain *chain) { _chains.push_back(chain); }
		//! @brief Adds action to Key list.
		inline void addAction(const ActionEvent &key) { _keys.push_back(key); }
		
		Chain *findChain(XKeyEvent *ev);
		ActionEvent *findAction(XKeyEvent *ev);

	private:
		uint _mod, _key;

		std::list<Chain*> _chains;
		std::list<ActionEvent> _keys;
	};

	KeyGrabber(PScreen *scr);
	~KeyGrabber(void);

	//! @brief Returns the KeyGrabber instance pointer.
	static KeyGrabber *instance(void) { return _instance; }

	void load(const std::string &file);
	void grabKeys(Window win);
	void ungrabKeys(Window win);

	ActionEvent *findAction(XKeyEvent *ev, PWinObj::Type type);
	ActionEvent *findMoveResizeAction(XKeyEvent *ev);

private:
	void grabKey(Window win, uint mod, uint key);

	void parseGlobalChain(CfgParser::Entry *op_section,
                        KeyGrabber::Chain *chain);
	void parseMoveResizeChain(CfgParser::Entry *op_section,
                            KeyGrabber::Chain *chain);
	void parseCmdDialogChain(CfgParser::Entry *op_section,
                           KeyGrabber::Chain *chain);
#ifdef MENUS
	void parseMenuChain(CfgParser::Entry *op_section,
                      KeyGrabber::Chain *chain);
#endif // MENUS

	ActionEvent *findAction(XKeyEvent *ev, KeyGrabber::Chain *chain);

private:
	PScreen *_scr;

#ifdef MENUS
	KeyGrabber::Chain _menu_chain;
#endif // MENUS
	KeyGrabber::Chain _global_chain;
	KeyGrabber::Chain _moveresize_chain;
	KeyGrabber::Chain _cmd_d_chain;

	uint _num_lock;
	uint _scroll_lock;

	static KeyGrabber *_instance;
};

#endif // _KEYGRABBER_HH_
