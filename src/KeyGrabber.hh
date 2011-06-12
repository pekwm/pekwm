//
// KeyGrabber.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _KEYGRABBER_HH_
#define _KEYGRABBER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"

#include "Action.hh"
#include "CfgParser.hh"
#include "PWinObj.hh"

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

    KeyGrabber(void);
    ~KeyGrabber(void);

    //! @brief Returns the KeyGrabber instance pointer.
    static KeyGrabber *instance(void) { return _instance; }

    bool load(const std::string &file, bool force=false);
    void grabKeys(Window win);
    void ungrabKeys(Window win);

    ActionEvent *findAction(XKeyEvent *ev, PWinObj::Type type);
    ActionEvent *findMoveResizeAction(XKeyEvent *ev);

private:
    void grabKey(Window win, uint mod, uint key);

    void parseGlobalChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);
    void parseMoveResizeChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);
    void parseInputDialogChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);
    void parseMenuChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);

    ActionEvent *findAction(XKeyEvent *ev, KeyGrabber::Chain *chain);

    std::map <std::string, time_t> _cfg_state; /**< Map of file mtime for all files touched by a configuration. */

    KeyGrabber::Chain _menu_chain;
    KeyGrabber::Chain _global_chain;
    KeyGrabber::Chain _moveresize_chain;
    KeyGrabber::Chain _input_dialog_chain;

    uint _num_lock;
    uint _scroll_lock;

    static KeyGrabber *_instance;
};

#endif // _KEYGRABBER_HH_
