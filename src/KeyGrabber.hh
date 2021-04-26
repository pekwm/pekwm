//
// KeyGrabber.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_KEYGRABBER_HH_
#define _PEKWM_KEYGRABBER_HH_

#include "config.h"

#include "pekwm.hh"

#include "Action.hh"
#include "CfgParser.hh"
#include "PWinObj.hh"

#include <string>

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

        //! @brief Returns vector of Chains that follows.
        const std::vector<Chain*> &getChains(void) const { return _chains; }
        //! @brief Returns vector of Keys in chain.
        const std::vector<ActionEvent> &getKeys(void) const { return _keys; }

        //! @brief Adds chain to Chain vector.
        inline void addChain(Chain *chain) { _chains.push_back(chain); }
        //! @brief Adds action to Key vector.
        inline void addAction(const ActionEvent &key) { _keys.push_back(key); }

        Chain *findChain(XKeyEvent *ev, bool &matched);
        ActionEvent *findAction(XKeyEvent *ev, bool &matched);

    private:
        uint _mod, _key;

        std::vector<Chain*> _chains;
        std::vector<ActionEvent> _keys;
    };

    KeyGrabber(void);
    ~KeyGrabber(void);

    bool load(const std::string &file, bool force=false);
    void grabKeys(Window win);
    void ungrabKeys(Window win);

    ActionEvent *findAction(XKeyEvent *ev, PWinObj::Type type, bool &matched);
    ActionEvent *findMoveResizeAction(XKeyEvent *ev);

private:
    void grabKey(Window win, uint mod, uint key);

    void parseGlobalChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);
    void parseMoveResizeChain(CfgParser::Entry *section,
                              KeyGrabber::Chain *chain);
    void parseInputDialogChain(CfgParser::Entry *section,
                               KeyGrabber::Chain *chain);
    void parseMenuChain(CfgParser::Entry *section, KeyGrabber::Chain *chain);

    ActionEvent *findAction(XKeyEvent *ev, KeyGrabber::Chain *chain,
                            bool &matched);

    TimeFiles _cfg_files;

    KeyGrabber::Chain _menu_chain;
    KeyGrabber::Chain _global_chain;
    KeyGrabber::Chain _moveresize_chain;
    KeyGrabber::Chain _input_dialog_chain;

    uint _num_lock;
    uint _scroll_lock;
};

namespace pekwm
{
    KeyGrabber* keyGrabber();
}

#endif // _PEKWM_KEYGRABBER_HH_
