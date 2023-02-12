//
// TkImage.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TK_IMAGE_HH_
#define _PEKWM_TK_IMAGE_HH_

#include "TkWidget.hh"
#include "PImage.hh"

class TkImage : public TkWidget {
public:
	TkImage(Theme::DialogData* data, PWinObj& parent, PImage* image)
		: TkWidget(data, parent),
		  _image(image)
	{
	}
	virtual ~TkImage(void) { }

	virtual uint widthReq(void) const
	{
		return _image->getWidth();
	}

	virtual uint heightReq(uint width) const
	{
		if (_image->getWidth() > width) {
			float aspect = float(_image->getWidth())
					     / _image->getHeight();
			return static_cast<uint>(width / aspect);
		}
		return _image->getHeight();
	}

	virtual void render(Render &rend, PSurface&)
	{
		if (_image->getWidth() > _gm.width) {
			float aspect = float(_image->getWidth())
					     / _image->getHeight();
			_image->draw(rend, _gm.x, _gm.y, _gm.width,
				     static_cast<uint>(_gm.width / aspect));
		} else {
			// render image centered on available width
			uint x = (_gm.width - _image->getWidth()) / 2;
			_image->draw(rend,
				     x, _gm.y,
				     _image->getWidth(), _image->getHeight());
		}
	}

private:
	PImage *_image;
};

#endif // _PEKWM_TK_IMAGE_HH_
