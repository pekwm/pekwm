//
// WORefMenu.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_WOREFMENU_HH_
#define _PEKWM_WOREFMENU_HH_

#include "config.h"

#include <string>

#include "PMenu.hh"
#include "PWinObjReference.hh"

class WORefMenu : public PMenu,
		  public PWinObjReference
{
protected:
	WORefMenu(const std::string &title, const std::string &name,
		  const std::string &decor_name = "MENU");

public:
	virtual ~WORefMenu(void);

	virtual void notify(Observable *observable, Observation *observation);

	virtual void setWORef(PWinObj *wo_ref);

protected:
	std::string _title_base;
	std::string _title_pre;
	std::string _title_post;
};

#endif // _PEKWM_WOREFMENU_HH_
