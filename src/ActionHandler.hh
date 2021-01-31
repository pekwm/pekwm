//
// ActionHandler.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "Action.hh"
#include "AppCtrl.hh"
#include "CmdDialog.hh"
#include "FocusCtrl.hh"
#include "SearchDialog.hh"

#include <string>
#include <map>

class Client;
class Frame;
class PMenu;
class WindowManager;

class ActionHandler
{
public:
    ActionHandler(AppCtrl* app_ctrl, FocusCtrl* focus_ctrl);
    ~ActionHandler(void);

    void handleAction(const ActionPerformed &ap);

    static bool checkAEThreshold(int x, int y, int x_t, int y_t, uint t);
    static ActionEvent *findMouseAction(uint button, uint mod,
                                        MouseEventType type,
                                        std::vector<ActionEvent> *actions);

private:
    void lookupWindowObjects(PWinObj **wo, Client **client, Frame **frame,
                             PMenu **menu, PDecor **decor);
    void handleStateAction(const Action &action, PWinObj *wo,
                           Client *client, Frame *frame);

    void actionExec(Client *client, const std::string &command, bool use_shell);
    void actionFindClient(const std::wstring &title);
    void actionGotoClientID(uint id);
    void actionGotoWorkspace(uint workspace, bool warp);
    void actionSendToWorkspace(PDecor *decor, int direction);
    void actionWarpToWorkspace(PDecor *decor, uint direction);
    void actionFocusToggle(uint button, uint raise, int off,
                           bool show_iconified, bool mru);
    void actionFocusDirectional(PWinObj *wo, DirectionType dir, bool raise);
    bool actionSendKey(PWinObj *wo, const std::string &key_str);
    static void actionSetOpacity(PWinObj *client, PWinObj *frame,
                                 uint focus, uint unfocus);
    void actionShowMenu(const std::string &name, bool stick, uint e_type,
                        PWinObj *wo_ref);
    void actionShowInputDialog(InputDialog *dialog, const std::string &initial,
                               Frame *frame, PWinObj *wo);
    bool actionWarpPointer(int x, int y);

    // action helpers
    Client *findClientFromTitle(const std::wstring &title);
    void gotoClient(Client *client);

    PMenu *createNextPrevMenu(bool show_iconified, bool mru);
    bool createMenuInclude(Frame *frame, bool show_iconified);

    void initSendKeyEvent(XEvent &ev, PWinObj *wo);

    void attachMarked(Frame *frame);
    void attachInNextPrevFrame(Client *client, bool frame, bool next);

    int calcWorkspaceNum(const Action& action);

private:
    AppCtrl* _app_ctrl;
    FocusCtrl* _focus_ctrl;
    CmdDialog _cmd_dialog;
    SearchDialog _search_dialog;

    /** Map translating state modifiers to keycode. */
    std::map<uint, uint> _state_to_keycode;
};

namespace pekwm
{
    ActionHandler* actionHandler();
}
