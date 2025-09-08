//
// X11_XRandr.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_X11_XRANDR_HH_
#define _PEKWM_X11_XRANDR_HH_

#include "config.h"

#include "Compat.hh"
#include "Mem.hh"
#include "X11.hh"

extern "C" {
#ifdef PEKWM_HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#else // !PEKWM_HAVE_XRANDR
typedef XID RROutput;
typedef XID RRCrtc;
typedef XID RRMode;
typedef unsigned short Rotation;
#define RR_Rotate_0 1
#define RR_Rotate_90 2
#define RR_Rotate_180 4
#define RR_Rotate_270 8
#endif // PEKWM_HAVE_XRANDR
}

class X11_XRandr_ModeInfo {
public:
	virtual ~X11_XRandr_ModeInfo() { }

	virtual bool good() const = 0;
	virtual RRMode getId() const = 0;
	virtual const char *getName() const = 0;
	virtual double getRefresh() const = 0;
};

class X11_XRandr_OutputInfo {
public:
	virtual ~X11_XRandr_OutputInfo() { }

	virtual RROutput getOutput() const = 0;
	virtual const char *getName() const = 0;
	virtual std::string getEdid() = 0;
	virtual bool isConnected() const = 0;

	virtual bool haveCrtc() const = 0;
	virtual bool isCrtcUsed() const = 0;
	virtual RRCrtc getCrtcId() const = 0;
	virtual RRMode getCrtcModeId() const = 0;
	virtual int getCrtcX() const = 0;
	virtual int getCrtcY() const = 0;
	virtual Rotation getCrtcRotation() const = 0;
};

class X11_XRandr_ScreenResources {
public:
	virtual ~X11_XRandr_ScreenResources() { }

	virtual bool empty() const = 0;
	virtual int size() const = 0;

	virtual X11_XRandr_OutputInfo *getOutput(int n) = 0;
	virtual X11_XRandr_OutputInfo *getOutput(const std::string &name) = 0;
	virtual bool haveOutput(const std::string &name) const = 0;

	virtual X11_XRandr_ModeInfo *getModeInfo(RRMode id) const = 0;
	virtual X11_XRandr_ModeInfo *findModeInfo(const std::string &name,
						  double refresh) = 0;

	virtual bool setCrtcConfig(X11_XRandr_OutputInfo *oi, int x, int y,
				   X11_XRandr_ModeInfo *mi,
				   Rotation rotation) = 0;
};

X11_XRandr_ScreenResources *mkX11_XRandr_ScreenResouces();
void X11_XRandr_SetSize(int width, int height, int width_mm, int height_mm);

#endif // _PEKWM_X11_XRANDR_HH_
