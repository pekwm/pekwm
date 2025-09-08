//
// X11_XRandr.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Compat.hh"
#include "Debug.hh"
#include "X11_XRandr.hh"

#ifdef PEKWM_HAVE_XRANDR

class X11_XRandr_ModeInfoImpl : public X11_XRandr_ModeInfo {
public:
	X11_XRandr_ModeInfoImpl(XRRModeInfo *mi)
		: _mi(mi)
	{
	}

	virtual bool good() const { return _mi != nullptr; }
	virtual RRMode getId() const { return _mi ? _mi->id : None; }
	virtual const char *getName() const { return _mi ? _mi->name : ""; }

	virtual double getRefresh() const {
		if (_mi && _mi->hTotal && _mi->vTotal) {
			return (double)_mi->dotClock
				/ ((double)_mi->hTotal * _mi->vTotal);
		}
		return 0.0;
	}

private:
	XRRModeInfo *_mi;
};

class X11_XRandr_OutputInfoImpl : public X11_XRandr_OutputInfo {
public:
	X11_XRandr_OutputInfoImpl()
		: _output(None),
		  _oi(nullptr),
		  _ci(nullptr)
	{
	}

	X11_XRandr_OutputInfoImpl(RROutput output, XRROutputInfo *oi,
				  XRRCrtcInfo *ci)
		: _output(output),
		  _oi(oi),
		  _ci(ci)
	{
	}

	virtual RROutput getOutput() const { return _output; }
	virtual const char *getName() const { return _oi ? _oi->name : ""; }
	virtual std::string getEdid() {
		Atom actual_type;
		int actual_format;
		unsigned long size, bytes_after;
		unsigned char *edid_data;
		if (XRRGetOutputProperty(X11::getDpy(), _output,
					 X11::getAtom(EDID),
					 0, 256, False, False, AnyPropertyType,
					 &actual_type, &actual_format, &size,
					 &bytes_after, &edid_data)
			!= Success) {
			return "";
		}
		std::string edid(reinterpret_cast<const char*>(edid_data),
				 size);
		XFree(edid_data);
		return edid;
	}

	virtual bool isConnected() const {
		return _oi && _oi->connection == RR_Connected;
	}

	virtual bool haveCrtc() const { return _ci != nullptr; }
	virtual bool isCrtcUsed() const {
		return _ci && _ci->noutput > 0;
	}
	virtual RRCrtc getCrtcId() const { return _oi ? _oi->crtc : None; }
	virtual RRMode getCrtcModeId() const { return _ci ? _ci->mode : None; }
	virtual int getCrtcX() const { return _ci ? _ci->x : 0; }
	virtual int getCrtcY() const { return _ci ? _ci->y : 0; }
	virtual Rotation getCrtcRotation() const {
		return _ci ? _ci->rotation : RR_Rotate_0;
	}

private:
	RROutput _output;
	XRROutputInfo *_oi;
	XRRCrtcInfo *_ci;
};

class X11_XRandr_ScreenResourcesImpl : public X11_XRandr_ScreenResources {
public:
	typedef std::map<std::string, X11_XRandr_OutputInfoImpl>
		string_to_output_info_map;

	X11_XRandr_ScreenResourcesImpl()
		: _sr(nullptr)
	{
		if (! X11::hasExtensionXRandr()) {
			P_TRACE("XRandr extension missing");
			return;
		}

		_sr = XRRGetScreenResources(X11::getDpy(), X11::getRoot());
		if (_sr == nullptr) {
			P_WARN("XRRGetScreenResources failed");
			return;
		}

		for (int i = 0; i < size(); i++) {
			const RROutput output = _sr->outputs[i];
			XRROutputInfo *oi =
				XRRGetOutputInfo(X11::getDpy(), _sr,
						 output);
			_ois.push_back(oi);
			XRRCrtcInfo *ci = nullptr;
			if (oi != nullptr && oi->crtc != None) {
				ci = XRRGetCrtcInfo(X11::getDpy(), _sr,
						    oi->crtc);
				_cis.push_back(ci);
			}

			_outputs.push_back(X11_XRandr_OutputInfoImpl(
				output, oi, ci));
			_name_to_output[_outputs.back().getName()] = 
				_outputs.back();
		}
	}

	virtual ~X11_XRandr_ScreenResourcesImpl()
	{
		std::vector<XRRCrtcInfo*>::iterator cit(_cis.begin());
		for (; cit != _cis.end(); ++cit) {
			if (*cit != nullptr) {
				XRRFreeCrtcInfo(*cit);
			}

		}
		std::vector<XRROutputInfo*>::iterator oit(_ois.begin());
		for (; oit != _ois.end(); ++oit) {
			if (*oit != nullptr) {
				XRRFreeOutputInfo(*oit);
			}
		}
		if (_sr) {
			XRRFreeScreenResources(_sr);
		}
	}

	virtual bool empty() const {
		return _sr == nullptr || _sr->noutput == 0;
	}
	virtual int size() const { return _sr ? _sr->noutput : 0; }

	virtual X11_XRandr_OutputInfo *getOutput(int n) {
		return &_outputs[n];
	}
	virtual X11_XRandr_OutputInfo *getOutput(const std::string &name) {
		return &_name_to_output[name];
	}
	virtual bool haveOutput(const std::string &name) const {
		string_to_output_info_map::const_iterator it =
			_name_to_output.find(name);
		return it != _name_to_output.end();
	}

	virtual X11_XRandr_ModeInfo *getModeInfo(RRMode id) const {
	       for (int i = 0; i < _sr->nmode; i++) {
			if (_sr->modes[i].id == id) {
				return new X11_XRandr_ModeInfoImpl(
					&_sr->modes[i]);
			}
	        }
		return nullptr;
	}

	virtual X11_XRandr_ModeInfo *findModeInfo(const std::string &name,
						  double refresh) {
		for (int i = 0; i < _sr->nmode; i++) {
			X11_XRandr_ModeInfoImpl mi(&_sr->modes[i]);
			if (name == mi.getName()
			    && (refresh == 0.0
				|| (refresh >= (mi.getRefresh() - 1.0)
				    && refresh <= (mi.getRefresh() + 1.0)))) {
				return new X11_XRandr_ModeInfoImpl(mi);
			}
		}
		return nullptr;
	}

	virtual bool setCrtcConfig(X11_XRandr_OutputInfo *oi, int x, int y,
				   X11_XRandr_ModeInfo *mi,
				   Rotation rotation) {
		RROutput output = oi->getOutput();
		RRCrtc crtc = oi->isCrtcUsed()
			? findFreeCrtc() : oi->getCrtcId();
		if (crtc == None) {
			P_WARN("no crtc found for " << oi->getName());
			return false;
		}

		Status status =
			XRRSetCrtcConfig(X11::getDpy(), _sr, crtc, CurrentTime,
					 x, y, mi->getId(), rotation,
					 &output, 1);
		if (status == RRSetConfigSuccess && crtc != oi->getCrtcId()) {
			// disable previous set CRTC mode
			XRRSetCrtcConfig(X11::getDpy(), _sr, oi->getCrtcId(),
					 CurrentTime, 0, 0, None, RR_Rotate_0,
					 NULL, 0);
		} else if (status != RRSetConfigSuccess) {
			P_WARN("failed to set configuration on "
				<< oi->getName() << ": " << errString(status));
		}
		return status == RRSetConfigSuccess;
	}

private:
	static const char *errString(Status status) {
		switch (status) {
		case RRSetConfigInvalidConfigTime:
        	        return "invalid config time";
		case RRSetConfigInvalidTime:
        	        return "invalid time";
		case RRSetConfigFailed:
        	        return "configuration failed";
		default:
			return "unknown error";
		}
	}

	X11_XRandr_ScreenResourcesImpl(const X11_XRandr_ScreenResourcesImpl&);
	X11_XRandr_ScreenResourcesImpl& operator=(
			const X11_XRandr_ScreenResourcesImpl&);

	RRCrtc findFreeCrtc() {
		for (int i = 0; i < _sr->ncrtc; i++) {
			Destruct<XRRCrtcInfo> ci(
				XRRGetCrtcInfo(X11::getDpy(), _sr,
					       _sr->crtcs[i]),
					       false, XRRFreeCrtcInfo);
			if (*ci != nullptr && ci->noutput == 0) {
				return _sr->crtcs[i];
			}
		}
		return None;
	}

	XRRScreenResources *_sr;
	std::vector<XRROutputInfo*> _ois;
	std::vector<XRRCrtcInfo*> _cis;
	std::vector<X11_XRandr_OutputInfoImpl> _outputs;
	string_to_output_info_map _name_to_output;
};

void
X11_XRandr_SetSize(int width, int height, int width_mm, int height_mm)
{
	XRRSetScreenSize(X11::getDpy(), X11::getRoot(), width, height,
			 width_mm, height_mm);
}

#else // ! PEKWM_HAVE_XRANDR

class X11_XRandr_ScreenResourcesImpl : public X11_XRandr_ScreenResources {
public:
	virtual ~X11_XRandr_ScreenResourcesImpl() { }

	virtual bool empty() const { return true; }
	virtual int size() const { return 0; }

	virtual X11_XRandr_OutputInfo *getOutput(int) { return nullptr; }
	virtual X11_XRandr_OutputInfo *getOutput(const std::string&) {
		return nullptr;
	}
	virtual bool haveOutput(const std::string &) const { return false; }

	virtual X11_XRandr_ModeInfo *getModeInfo(RRMode id) const {
		return nullptr;
	}

	virtual X11_XRandr_ModeInfo *findModeInfo(const std::string &name,
						  double refresh) {
		return nullptr;
	}

	virtual bool setCrtcConfig(X11_XRandr_OutputInfo *oi, int x, int y,
				   X11_XRandr_ModeInfo *mi,
				   Rotation rotation) {
		return false;
	}
};

void
X11_XRandr_SetSize(int width, int height, int width_mm, int height_mm)
{
}

#endif // PEKWM_HAVE_XRANDR

X11_XRandr_ScreenResources*
mkX11_XRandr_ScreenResouces()
{
	return new X11_XRandr_ScreenResourcesImpl();
}
