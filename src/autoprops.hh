//
// autoprops.hh for pekwm
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

#ifndef _AUTOPROPS_HH_
#define _AUTOPROPS_HH_

#include "baseconfig.hh"

#include <string>
#include <list>

class Config; // forward

class AutoProps {
public:
	class ClassHint {
	public:
		ClassHint() : m_name("") , m_class(""), m_group("") { }
		ClassHint(const std::string &n, const std::string &c) :
			m_name(n) , m_class(c) , m_group(""){ }
		~ClassHint() { }

		inline const std::string &getName(void) const { return m_name; }
		inline const std::string &getClass(void) const { return m_class; }
		inline const std::string &getGroup(void) const { return m_group; }
		inline void setName(const std::string &n) { m_name = n; }
		inline void setClass(const std::string &c) { m_class = c; }
		inline void setGroup(const std::string &g) { m_group = g; }

		// TO-DO: jucky, messy stuff
		inline bool operator == (const ClassHint &c) const {
			if (m_group.size()) {
				if (m_group == c.m_group)
					return true;

			} else if (((m_name == "*") || (c.m_name == "*") ||
									(m_name == c.m_name)) &&
								 ((m_class == "*") || (c.m_class == "*") ||
									(m_class == c.m_class)))
				return true;
			return false;
		}

		inline bool operator != (const ClassHint &c) const {
			if (m_group.size()) {
				if (m_group == c.m_group)
					return false;
				return true;

			} else if (((m_name == "*") || (c.m_name == "*") ||
									(m_name == c.m_name)) &&
								 (m_class == "*") || (c.m_class == "*") ||
								 (m_name == c.m_name) || (m_class == c.m_class))
				return false;
			return true;
		}

	private:
		std::string m_name;
		std::string m_class;
		std::string m_group;
	};

	enum Property {
		STICKY = (1<<1),
		SHADED = (1<<2),
		MAXIMIZED_VERTICAL = (1<<3),
		MAXIMIZED_HORIZONTAL = (1<<4),
		ICONIFIED = (1<<5),
		BORDER = (1<<6),
		TITLEBAR = (1<<7),
		POSITION = (1<<8),
		SIZE = (1<<9),
		LAYER = (1<<10),
		AUTO_GROUP = (1<<11),
		WORKSPACE = (1<<12),
		APPLY_ON_START = (1<<13),
		APPLY_ON_RELOAD = (1<<14),
		APPLY_ON_WORKSPACE_CHANGE = (1<<15),
		APPLY_ON_TRANSIENT = (1<<16),

		GROUP,
		PROPERTY,
		NO_PROPERTY
	};

	class AutoPropData {
	public:
		AutoPropData() : prop_mask(0) { }
		~AutoPropData() { }

		inline bool setSticky(void) const { return (prop_mask&STICKY); }
		inline bool setShaded(void) const { return (prop_mask&SHADED); }
		inline bool setMaximizedVertical(void) const {
			return (prop_mask&MAXIMIZED_VERTICAL); }
		inline bool setMaximizedHorizontal(void) const {
			return (prop_mask&MAXIMIZED_HORIZONTAL); }
		inline bool setIconified(void) const { return (prop_mask&ICONIFIED); }
		inline bool setBorder(void) const { return (prop_mask&BORDER); }
		inline bool setTitlebar(void) const { return (prop_mask&TITLEBAR); }
		inline bool setPosition(void) const { return (prop_mask&POSITION); }
		inline bool setSize(void) const { return (prop_mask&SIZE); }
		inline bool setLayer(void) const { return (prop_mask&LAYER); }
		inline bool autoGroup(void) const { return (prop_mask&AUTO_GROUP); }
		inline bool setWorkspace(void) const { return (prop_mask&WORKSPACE); }

		inline bool applyOnTransient(void) const {
			return (prop_mask&APPLY_ON_TRANSIENT);
		}

		inline bool applyOnStart(void) const { return (prop_mask&APPLY_ON_START); }
		inline bool applyOnReload(void) const {
			return (prop_mask&APPLY_ON_RELOAD); }
		inline bool applyOnWorkspaceChange(void) const {
			return (prop_mask&APPLY_ON_WORKSPACE_CHANGE); }

	public:
		ClassHint class_hint;

		int prop_mask;
		std::list<unsigned int> workspaces;

		bool sticky;
		bool shaded;
		bool maximized_vertical, maximized_horizontal;
		bool iconified;
		bool border;
		bool titlebar;

		int x, y;
		unsigned int width, height;
		unsigned int layer;
		unsigned int auto_group;
		unsigned int workspace;

		bool apply_on_transient;
		bool apply_on_start, apply_on_reload, apply_on_workspace_change;
	};

	AutoProps(Config *cfg);
	~AutoProps();

	void loadConfig(void);
	AutoPropData *getAutoProp(const ClassHint &class_hint,
														int workspace = -1,
														int type = -1);

private:
	Property getProperty(const std::string &property_name);
	void parseProperty(BaseConfig::CfgSection *cs, std::list<unsigned int> *ws);
private:
	Config *cfg;
	std::list<AutoPropData> m_prop_list;

	struct autoproplist_item {
		const char *name;
		Property property;

		inline bool operator == (std::string s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static autoproplist_item m_autoproplist[];
};

#endif // _AUTOPROPS_HH_
