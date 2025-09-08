//
// SysMonitorConfig.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <fstream>
#include <sstream>

#include "Debug.hh"
#include "SysMonitorConfig.hh"
#include "X11.hh"

SysMonitorConfig::SysMonitorConfig()
	: _sr(mkX11_XRandr_ScreenResouces())
{
}

SysMonitorConfig::~SysMonitorConfig()
{
	delete _sr;
}

bool
SysMonitorConfig::load(const std::string &path)
{
	CfgParser cfg(CfgParserOpt(""));
	if (! cfg.parse(path, CfgParserSource::SOURCE_FILE, true)) {
		return false;
	}
	return load(cfg.getEntryRoot());
}

bool
SysMonitorConfig::load(CfgParser::Entry *section)
{
	_config.clear();
	if (section == nullptr) {
		return false;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		if (! pekwm::ascii_ncase_equal((*it)->getName(), "MONITORS")) {
			continue;
		}
		MonitorsConfig monitors = loadMonitors((*it)->getSection());
		_config[monitors.getId()] = monitors;
	}
	return true;
}

SysMonitorConfig::MonitorsConfig
SysMonitorConfig::loadMonitors(CfgParser::Entry *section)
{
	if (section == nullptr) {
		return MonitorsConfig();
	}

	CfgParserKeys keys;
	int width, height, width_mm, height_mm;
	keys.add_numeric<int>("WIDTH", width, 0);
	keys.add_numeric<int>("HEIGHT", height, 0);
	keys.add_numeric<int>("WIDTHMM", width_mm, 0);
	keys.add_numeric<int>("HEIGHTMM", height_mm, 0);
	section->parseKeyValues(keys.begin(), keys.end());

	MonitorsConfig monitors(width, height, width_mm, height_mm);
	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		if (pekwm::ascii_ncase_equal((*it)->getName(), "MONITOR")
		    && (*it)->getSection() != nullptr) {
			MonitorConfig monitor =
				loadMonitor((*it)->getSection());
			monitors.add(monitor);
		}
	}
	return monitors;
}

static Rotation
_rotation_from_int(int rotation)
{
	switch (rotation) {
	case 90:
		return RR_Rotate_90;
	case 180:
		return RR_Rotate_180;
	case 270:
		return RR_Rotate_270;
	default:
		return RR_Rotate_0;
	}
}

static int
_rotation_to_int(Rotation rotation)
{
	switch (rotation) {
	case RR_Rotate_90:
		return 90;
	case RR_Rotate_180:
		return 180;
	case RR_Rotate_270:
		return 270;
	default:
		return 0;
	}
}

SysMonitorConfig::MonitorConfig
SysMonitorConfig::loadMonitor(CfgParser::Entry *section)
{
	CfgParserKeys keys;
	std::string mode_name, edid;
	int pos_x, pos_y, rotation_int;
	double refresh;
	keys.add_string("MODENAME", mode_name);
	keys.add_string("EDID", edid);
	keys.add_numeric<double>("REFRESHRATE", refresh, 0.0, 0.0);
	keys.add_numeric<int>("POSX", pos_x, 0);
	keys.add_numeric<int>("POSY", pos_y, 0);
	keys.add_numeric<int>("ROTATION", rotation_int, 0, 0, 270);
	section->parseKeyValues(keys.begin(), keys.end());
	return MonitorConfig(section->getValue(), mode_name, edid,
			     refresh, pos_x, pos_y,
			     _rotation_from_int(rotation_int));
}

bool
SysMonitorConfig::save(const std::string &path)
{
	std::ofstream ofs(path.c_str(), std::ios::out | std::ios::trunc);
	if (ofs.good()) {
		return save(ofs);
	}
	return false;
}

bool
SysMonitorConfig::save(std::ostream &os)
{
	std::map<std::string, MonitorsConfig>::iterator it(_config.begin());
	for (; it != _config.end(); ++it) {
		if (it != _config.begin()) {
			os << std::endl;
		}
		saveMonitors(os, it->second);
	}
	return true;
}

void
SysMonitorConfig::saveMonitors(std::ostream &os,
			       const MonitorsConfig &msc) const
{
	os << "Monitors = \"" << msc.getId() << "\" {" << std::endl
	   << "\tWidth = \"" << msc.getWidth() << "\"" << std::endl
	   << "\tHeight = \"" << msc.getHeight() << "\"" << std::endl
	   << "\tWidthMm = \"" << msc.getWidthMm() << "\"" << std::endl
	   << "\tHeightMm = \"" << msc.getHeightMm() << "\"" << std::endl;
	MonitorsConfig::const_iterator it(msc.begin());
	for (; it != msc.end(); ++it) {
		if (it != msc.begin()) {
			os << std::endl;
		}
		saveMonitor(os, *it);
	}
	os << "}" << std::endl;
}

void
SysMonitorConfig::saveMonitor(std::ostream &os, const MonitorConfig &mc) const
{
	os << "\tMonitor = \"" << mc.getName() << "\" {" << std::endl
	   << "\t\tModeName = \"" << mc.getModeName() << "\"" << std::endl
	   << "\t\tEdid = \"" << mc.getEdidMd5() << "\"" << std::endl
	   << "\t\tRefreshRate = \""
	   << pekwm::to_string(mc.getRefreshRate(), 2)
	   << "\"" << std::endl
	   << "\t\tPosX = \"" << mc.getPosX() << "\"" << std::endl
	   << "\t\tPosY = \"" << mc.getPosY() << "\"" << std::endl
	   << "\t\tRotate = \""
	   << _rotation_to_int(mc.getRotation())
	   << "\"" << std::endl
	   << "\t}" << std::endl;
}

/**
 * Construct a MonitorsConfig object from the current Xrandr configuration.
 */
bool
SysMonitorConfig::mk(MonitorsConfig &monitors)
{
	Destruct<X11_XRandr_ScreenResources> sr(mkX11_XRandr_ScreenResouces());
	if (sr->empty()) {
		std::ostringstream os;
		os << X11::getWidth() << "x" << X11::getHeight();
		MonitorConfig monitor("X11", os.str(), "", 0.0, 0, 0,
				      RR_Rotate_0);
		monitors.add(monitor);
		return true;
	}

	monitors.setWidth(X11::getWidth());
	monitors.setHeight(X11::getHeight());
	monitors.setWidthMm(X11::getWidthMm());
	monitors.setHeightMm(X11::getHeightMm());

	for (int i = 0; i < sr->size(); i++) {
		X11_XRandr_OutputInfo *oi = sr->getOutput(i);
		Destruct<X11_XRandr_ModeInfo> mi(
			sr->getModeInfo(oi->getCrtcModeId()));
		if (! oi->isConnected() || ! oi->haveCrtc() || *mi == nullptr) {
			continue;
		}

		Md5 edidMd5(oi->getEdid());
		monitors.add(
			MonitorConfig(oi->getName(), mi->getName(),
				      edidMd5.hexDigest(), mi->getRefresh(),
				      oi->getCrtcX(), oi->getCrtcY(),
				      oi->getCrtcRotation()));
	}
	return true;
}

std::string
SysMonitorConfig::mkId()
{
	if (! X11::hasExtensionXRandr()) {
		return "X11";
	}

	Md5 md5;
	Destruct<X11_XRandr_ScreenResources> sr(mkX11_XRandr_ScreenResouces());
	for (int i = 0; i < sr->size(); i++) {
		X11_XRandr_OutputInfo *oi = sr->getOutput(i);
		if (oi->isConnected()) {
			md5.update(oi->getName());
			Md5 edidMd5(oi->getEdid());
			md5.update(edidMd5.hexDigest());
		}
	}
	md5.finalize();
	return md5.hexDigest();
}

bool
SysMonitorConfig::find(MonitorsConfig &monitors)
{
	std::string id = mkId();
	if (id.empty()) {
		return false;
	}
	std::map<std::string, MonitorsConfig>::iterator it(_config.find(id));
	if (it == _config.end()) {
		P_TRACE("did not find a configuration for id " << id
			<< " looking in " << _config.size()
			<< " configurations");
		return false;
	}
	monitors = it->second;
	return true;
}

bool
SysMonitorConfig::apply(const MonitorsConfig &monitors)
{
	Destruct<X11_XRandr_ScreenResources> sr(mkX11_XRandr_ScreenResouces());
	if (sr->empty()) {
		return false;
	}

	MonitorsConfig::const_iterator it(monitors.begin());
	for (; it != monitors.end(); ++it) {
		if (! applyMonitor(*sr, *it)) {
			return false;
		}
	}

	if (monitors.haveSize()) {
		X11_XRandr_SetSize(monitors.getWidth(), monitors.getHeight(),
				   monitors.getWidthMm(),
				   monitors.getHeightMm());
	}

	return true;
}

bool
SysMonitorConfig::applyMonitor(X11_XRandr_ScreenResources *sr,
			       const MonitorConfig &mc)
{
	Destruct<X11_XRandr_ModeInfo> mi(
		sr->findModeInfo(mc.getModeName(), mc.getRefreshRate()));
	if (*mi == nullptr) {
		P_LOG("mode " << mc.getModeName() << "@"
		       << mc.getRefreshRate() << " not found for "
		       << "output " << mc.getName());
		return false;
	}

	if (! sr->haveOutput(mc.getName())) {
		P_LOG("output " << mc.getName() << " not found");
		return false;
	}
	X11_XRandr_OutputInfo *oi = sr->getOutput(mc.getName());
	if (! oi->isConnected()) {
		P_LOG("output " << oi->getName() << " not connected");
		return false;
	}
	return sr->setCrtcConfig(oi, mc.getPosX(), mc.getPosY(), *mi,
				 static_cast<Rotation>(mc.getRotation()));
}

bool
SysMonitorConfig::autoConfig()
{
	Destruct<X11_XRandr_ScreenResources>
		newSr(mkX11_XRandr_ScreenResouces());
	if (! X11::hasExtensionXRandr() || newSr->empty()) {
		return false;
	}

	for (int i = 0; i < newSr->size(); i++) {
		X11_XRandr_OutputInfo *oi = newSr->getOutput(i);
		if (oi->isCrtcUsed()) {
			continue;
		}
	}
	return true;
}
