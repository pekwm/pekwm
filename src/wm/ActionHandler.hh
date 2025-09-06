//
// ActionHandler.hh for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_ACTIONHANDLER_HH_
#define _PEKWM_ACTIONHANDLER_HH_

#include "config.h"

#include "pekwm.hh"
#include "AppCtrl.hh"
#include "CmdDialog.hh"
#include "EventLoop.hh"
#include "SearchDialog.hh"

#include "Os.hh"
#include "tk/Action.hh"

#include <string>
#include <map>

class Client;
class Frame;
class PMenu;

class ActionHandler
{
public:
	ActionHandler(AppCtrl* app_ctrl, EventLoop* event_loop,
		      Os *os);
	~ActionHandler(void);

	void handleAction(const ActionPerformed *ap);

	static bool checkAEThreshold(int x, int y, int x_t, int y_t, uint t);
	static ActionEvent *findMouseAction(uint button, uint mod,
					    MouseEventType type,
					    std::vector<ActionEvent> *actions);

	void setSysProcess(ChildProcess *sys_process)
	{
		_sys_process = sys_process;
	}

	static bool fillEdgeGeometry(const Geometry &head,
				     OrientationType edge, int percent,
				     Geometry &gm);
	static void actionDetachClientSplitHorz(PWinObj *oldf, PWinObj *newf,
						float new_aspect);
	static void actionDetachClientSplitVert(PWinObj *oldf, PWinObj *newf,
						float new_aspect);

private:
	void handleAction(const ActionPerformed* ap, ActionEvent::it it);
	bool handleWoAction(const ActionPerformed* ap, ActionEvent::it it);
	bool handleFrameAction(const ActionPerformed* ap, ActionEvent::it it,
			       Client* client, Frame* frame);
	bool handleMenuAction(const ActionPerformed* ap, ActionEvent::it it,
			      PMenu* menu);
	bool handleDecorAction(const ActionPerformed* ap, ActionEvent::it it,
			       PDecor* decor);
	bool handleAnyAction(const ActionPerformed* ap, ActionEvent::it it,
			     Client* client, Frame* frame);

	void lookupWindowObjects(PWinObj **wo, Client **client, Frame **frame,
				 PMenu **menu, PDecor **decor);
	void handleStateAction(const Action &action, PWinObj *wo,
			       Client *client, Frame *frame);

	void actionExec(Client *client, const std::string &command,
			bool use_shell);
	void actionSetenv(const std::string &name, const std::string &value);
	bool actionMoveToEdge(PWinObj *wo, OrientationType edge);
	bool actionFillEdge(Frame *frame, OrientationType edge,
			    int percent);
	bool isOrientationOpposites(OrientationType lhs, OrientationType rhs);
	void actionFindClient(const std::string &title);
	void actionGotoClientID(uint id);
	void actionGotoWorkspace(const Action &action, int type);
	void actionGroupingDrag(const ActionPerformed *ap,
				Frame *frame, Client *client, bool behind);
	void actionResize(const ActionPerformed *ap,
			  Frame *frame, Client *client, BorderPosition pos);
	void actionMoveResize(PDecor *decor);
	void actionSendToWorkspace(PDecor *decor, bool focus, int direction);
	void actionWarpToWorkspace(const ActionPerformed *ap, PDecor *decor,
				   uint direction);
	void actionFocusWithSelector(const Action &action);
	void actionFocusDirectional(PWinObj *wo, DirectionType dir, bool raise);
	bool actionSendKey(PWinObj *wo, const std::string &key_str);
	static void actionSetOpacity(PWinObj *client, PWinObj *frame,
				     uint focus, uint unfocus);
	void actionShowMenu(const std::string &name, bool stick, uint e_type,
			    PWinObj *wo_ref);
	void actionShowInputDialog(InputDialog *dialog,
				   const std::string &initial,
				   Frame *frame, PWinObj *wo);
	bool actionWarpPointer(int x, int y);
	void actionSys(const std::string &cmd);
	void actionWmSet(const std::string &args_str);
	void actionWmSetScale(const std::vector<std::string> &args);

	// action helpers
	Client *findClientFromTitle(const std::string &title);
	void gotoClient(Client *client);

	void initSendKeyEvent(XEvent &ev, PWinObj *wo);

	void actionDetachClientSplit(Frame *frame, Client *client, int percent,
				     bool horz);
	void attachMarked(Frame *frame);
	void attachInNextPrevFrame(Client *client, bool frame, bool next);

	int calcWorkspaceNum(const Action& action, int index = 0);

	void setEventHandler(EventHandler *event_handler);

private:
	AppCtrl* _app_ctrl;
	EventLoop* _event_loop;
	Os* _os;
	CmdDialog _cmd_dialog;
	SearchDialog _search_dialog;
	ChildProcess *_sys_process;

	/** Map translating state modifiers to keycode. */
	std::map<uint, uint> _state_to_keycode;
};

namespace pekwm
{
	ActionHandler* actionHandler();
}

#endif // _PEKWM_ACTIONHANDLER_HH_
