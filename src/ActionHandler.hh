//
// ActionHandler.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _ACTIONHANDLER_HH_
#define _ACTIONHANDLER_HH_

#include "config.h"

#include "pekwm.hh"
#include "Action.hh"
#include "InputDialog.hh"

#include <string>
#include <map>

class Client;
class Frame;
class PWinObj;
class PDecor;
class PMenu;

class ActionHandler
{
public:
    ActionHandler(void);
    ~ActionHandler(void);

    static inline ActionHandler *instance(void) { return _instance; }

    void handleAction(const ActionPerformed &ap);

    static bool checkAEThreshold(int x, int y, int x_t, int y_t, uint t);
    static ActionEvent *findMouseAction(uint button, uint mod, MouseEventType type,
                                        vector<ActionEvent> *actions);
    
private:
    void lookupWindowObjects(PWinObj **wo, Client **client, Frame **frame,
                             PMenu **menu, PDecor **decor);
    void handleStateAction(const Action &action, PWinObj *wo, Client *client, Frame *frame);

    void actionExec(Client *client, const std::string &command);
    void actionFindClient(const std::wstring &title);
    void actionGotoClientID(uint id);
    void actionGotoWorkspace(uint workspace, bool warp);
    void actionSendToWorkspace(PDecor *decor, int direction);
    void actionWarpToWorkspace(PDecor *decor, uint direction);
    void actionFocusToggle(uint button, uint raise, int off, bool show_iconified, bool mru);
    void actionFocusDirectional(PWinObj *wo, DirectionType dir, bool raise);
    bool actionSendKey(PWinObj *wo, const std::string &key_str);
    static void actionSetOpacity(PWinObj *client, PWinObj *frame, uint focus, uint unfocus);
    void actionShowMenu(const std::string &name, bool stick, uint e_type, PWinObj *wo_ref);
    void actionShowInputDialog(InputDialog *dialog, const std::string &initial, Frame *frame, PWinObj *wo);

    // action helpers
    Client *findClientFromTitle(const std::wstring &title);
    void gotoClient(Client *client);

    PMenu *createNextPrevMenu(bool show_iconified, bool mru);
    bool createMenuInclude(Frame *frame, bool show_iconified);

    void initSendKeyEvent(XEvent &ev, PWinObj *wo);

private:
    std::map<uint, uint> _state_to_keycode; /**< Map translating state modifiers to keycode. */

    static ActionHandler *_instance; /**< Instance pointer for ActionHandler. */
};

#endif // _ACTIONHANDLER_HH_
