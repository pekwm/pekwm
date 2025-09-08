//
// SysMonitorConfig.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SYS_MONITOR_CONFIG_HH_
#define _PEKWM_SYS_MONITOR_CONFIG_HH_

#include <map>
#include <vector>
#include <string>

#include "config.h"

#include "CfgParser.hh"
#include "Md5.hh"
#include "Types.hh"
#include "X11_XRandr.hh"

class SysMonitorConfig {
public:
	class MonitorConfig {
	public:
		MonitorConfig()
			: _refresh_rate(0.0),
			  _pos_x(0),
			  _pos_y(0),
			  _rotation(RR_Rotate_0)
		{
		}

		MonitorConfig(const std::string &name,
			      const std::string &mode_name,
			      const std::string &edid_md5,
			      double refresh_rate, int pos_x, int pos_y,
			      Rotation rotation)
			: _name(name),
			  _mode_name(mode_name),
			  _edid_md5(edid_md5),
			  _refresh_rate(refresh_rate),
			  _pos_x(pos_x),
			  _pos_y(pos_y),
			  _rotation(rotation)
		{
		}

		const std::string &getName() const { return _name; }
		const std::string &getModeName() const { return _mode_name; }
		const std::string &getEdidMd5() const { return _edid_md5; }
		const double getRefreshRate() const { return _refresh_rate; }
		const int getPosX() const { return _pos_x; }
		const int getPosY() const { return _pos_y; }
		Rotation getRotation() const { return _rotation; }

	private:
		std::string _name;
		std::string _mode_name;
		std::string _edid_md5;
		double _refresh_rate;
		int _pos_x;
		int _pos_y;
		Rotation _rotation;
	};

	class MonitorsConfig {
	public:
		typedef std::vector<MonitorConfig> vector;
		typedef vector::iterator iterator;
		typedef vector::const_iterator const_iterator;

		MonitorsConfig(int width = 0, int height = 0,
			       int width_mm = 0, int height_mm = 0)
			: _width(width),
			  _height(height),
			  _width_mm(width_mm),
			  _height_mm(height_mm)
		{
		}

		const std::string &getId() const { return _id; }

		int getWidth() const { return _width; }
		int getHeight() const { return _height; }
		void setWidth(int width) { _width = width; }
		void setHeight(int height) { _height = height; }
		int getWidthMm() const { return _width_mm; }
		int getHeightMm() const { return _height_mm; }
		void setWidthMm(int width_mm) { _width_mm = width_mm; }
		void setHeightMm(int height_mm) { _height_mm = height_mm; }
		bool haveSize() const {
			return _width > 0 && _height > 0
				&& _width_mm > 0 && _height_mm > 0;
		}

		void add(const MonitorConfig &monitor) {
			_monitors.push_back(monitor);
			updateId();
		}

		const_iterator begin() const { return _monitors.begin(); }
		const_iterator end() const { return _monitors.end(); }

	private:
		void updateId() {
			Md5 md5;
			const_iterator it(begin());
			for (; it != end(); ++it) {
				md5.update(it->getName());
				md5.update(it->getEdidMd5());
			}
			md5.finalize();
			_id = md5.hexDigest();
		}

		std::string _id;
		int _width;
		int _height;
		int _width_mm;
		int _height_mm;
		vector _monitors;
	};

	SysMonitorConfig();
	~SysMonitorConfig();

	bool load(const std::string &path);
	bool load(CfgParser::Entry *section);

	bool save(const std::string &path);
	bool save(std::ostream &os);

	bool mk(MonitorsConfig &monitors);
	std::string mkId();
	bool find(MonitorsConfig &monitors);
	bool apply(const MonitorsConfig &monitors);
	bool autoConfig();

	void add(const MonitorsConfig &monitors) {
		_config[monitors.getId()] = monitors;
	}

private:
	MonitorsConfig loadMonitors(CfgParser::Entry *section);
	MonitorConfig loadMonitor(CfgParser::Entry *section);

	void saveMonitors(std::ostream &os, const MonitorsConfig &msc) const;
	void saveMonitor(std::ostream &os, const MonitorConfig &mc) const;

	bool applyMonitor(X11_XRandr_ScreenResources *sr,
			  const MonitorConfig &mc);

	/** screen resources representing the current screen configuration. */
	X11_XRandr_ScreenResources *_sr;
	/** current configuration. */
	MonitorsConfig _monitors;
	std::map<std::string, MonitorsConfig> _config;
};

#endif // _PEKWM_SYS_MONITOR_CONFIG_HH_
