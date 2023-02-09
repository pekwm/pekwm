#ifndef _PEKWM_TK_LAYOUT_HH_
#define _PEKWM_TK_LAYOUT_HH_

#include "TkWidget.hh"

class TkLayout {
public:
	void layout(const Geometry& gm, const std::vector<TkWidget*>& widgets,
		    Geometry& gm_out);
};

#endif // _PEKWM_TK_LAYOUT_HH_
