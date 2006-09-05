//
// screeninfo.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _SCREENINFO_HH_
#define _SCREENINFO_HH_

#include <X11/Xlib.h>

#ifdef XINERAMA
extern "C" {
#include <X11/extensions/Xinerama.h>
}
#endif

class ScreenInfo
{
public:
	ScreenInfo(Display *d);
	~ScreenInfo();

	inline Display *getDisplay(void) const { return dpy; }
	inline int getScreenNum(void) const { return m_screen; }
	inline Window getRoot(void) const { return m_root; }
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }

	inline int getDepth(void) const { return m_depth; }
	inline Visual *getVisual(void) { return m_visual; }
	inline Colormap getColormap(void) const { return m_colormap; }

	inline unsigned long getWhitePixel(void)
		const { return WhitePixel(dpy, m_screen); }
	inline unsigned long getBlackPixel(void)
		const { return BlackPixel(dpy, m_screen); }

	inline unsigned int getNumLock(void) const { return m_num_lock; }
	inline unsigned int getScrollLock(void) const { return m_scroll_lock; }

#ifdef XINERAMA
	struct HeadInfo {
		int x, y;
		unsigned int width, height;
	};

	inline bool hasXinerama(void) const { return m_has_xinerama; }
	inline int getNumHeads(void) const { return m_xinerama_num_heads; }

	unsigned int getHead(int x, int y);
	unsigned int getCurrHead(void);
	bool getHeadInfo(unsigned int head, HeadInfo &head_info);

#endif // XINERAMA

private:
	Display *dpy;

	int m_screen, m_depth;
	unsigned int m_width, m_height;

	Window m_root;
	Visual *m_visual;
	Colormap m_colormap;

	unsigned int m_num_lock;
	unsigned int m_scroll_lock;

#ifdef XINERAMA
	bool m_has_xinerama;
	unsigned int m_xinerama_last_head;
	int m_xinerama_num_heads;
	XineramaScreenInfo *m_xinerama_infos;
#endif // XINERAMA

};

#endif // _SCREENINFO_HH_
