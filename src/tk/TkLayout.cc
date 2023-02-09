#include "TkLayout.hh"

void
TkLayout::layout(const Geometry& gm, const std::vector<TkWidget*>& widgets,
		 Geometry& gm_out)
{
	// height is dependent on the available width, get requested
	// width first.
	uint width = gm.width;
	std::vector<TkWidget*>::const_iterator it = widgets.begin();
	for (; it != widgets.end(); ++it) {
		uint width_req = (*it)->widthReq();
		if (width_req && width_req > width) {
			width = width_req;
		}
	}

	uint y = 0;
	it = widgets.begin();
	for (; it != widgets.end(); ++it) {
		(*it)->place(0, y, width, 0);
		(*it)->setHeight((*it)->heightReq(width));
		y += (*it)->getHeight();
	}

	gm_out.width = std::max(width, gm.width);
	gm_out.height = std::max(y, gm.height);
}
