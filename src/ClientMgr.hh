//
// ClientMgr.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CLIENTMGR_HH_
#define _PEKWM_CLIENTMGR_HH_

#include "Action.hh"

class AutoProperty;
class Client;
class Frame;

class ClientMgr {
public:
	static Frame *findGroup(AutoProperty *ap);

	static bool isAllowGrouping(void) { return _allow_grouping; }
	static void setStateGlobalGrouping(StateAction sa) {
		if (ActionUtil::needToggle(sa, _allow_grouping)) {
			_allow_grouping = !_allow_grouping;
		}
	}

	static void familyRaiseLower(Client *client, bool raise);

private:
	static bool findGroupMatchProperty(Frame *frame, AutoProperty *property);
	static Frame* findGroupMatch(AutoProperty *property);

private:
	/** Global control used to disable grouping. */
	static bool _allow_grouping;
};

#endif // _PEKWM_CLIENTMGR_HH_
