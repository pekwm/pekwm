//
// PWinObjReference.hh for pekwm
// Copyright (C) 2009-2023 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PWINOBJREFERENCE_HH_
#define _PEKWM_PWINOBJREFERENCE_HH_

#include "config.h"

#include "PWinObj.hh"
#include "Observable.hh"

/**
 * Observer with a reference to a PWinObj.
 */
class PWinObjReference : public Observer {
public:
	PWinObjReference(PWinObj *wo_ref=nullptr)
		: _wo_ref(wo_ref)
	{
		updateObserver(nullptr, wo_ref);
	}
	virtual ~PWinObjReference(void)
	{
		updateObserver(_wo_ref, nullptr);
	}

	/** Notify about reference update, unset the reference. */
	virtual void notify(Observable *observable, Observation *observation) {
		if (observation == &PWinObj::pwin_obj_deleted
		    && observable == _wo_ref) {
			_wo_ref = nullptr;
		}
	}

	/** Returns the PWinObj reference. */
	PWinObj *getWORef(void) const { return _wo_ref; }

	/** Sets the PWinObj reference and add this as an observer. */
	virtual void setWORef(PWinObj *wo_ref) {
		if (_wo_ref != wo_ref) {
			updateObserver(_wo_ref, wo_ref);
			_wo_ref = wo_ref;
		}
	}

	void updateObserver(PWinObj* old_ref, PWinObj* new_ref)
	{
		ObserverMapping* om = pekwm::observerMapping();
		if (old_ref != nullptr) {
			om->removeObserver(old_ref, this);
		}
		if (new_ref != nullptr) {
			om->addObserver(new_ref, this, 100);
		}
	}

private:
	/** Window object reference. */
	PWinObj *_wo_ref;
};

#endif // _PEKWM_PWINOBJREFERENCE_HH_
