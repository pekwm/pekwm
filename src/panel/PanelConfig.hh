//
// PanelConfig.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PANEL_PANEL_CONFIG_HH_
#define _PEKWM_PANEL_PANEL_CONFIG_HH_

#include <string>
#include <vector>

#include "pekwm_panel.hh"
#include "CfgParser.hh"

extern "C" {
#ifdef PEKWM_HAVE_SYS_LIMITS_H
#include <sys/limits.h>
#else // ! PEKWM_HAVE_SYS_LIMITS_H
#include <limits.h>
#endif // PEKWM_HAVE_SYS_LIMITS_H
}

/**
 * Widgets to display.
 */
class WidgetConfig {
public:
	WidgetConfig(const std::string& name,
		     const std::vector<std::string>& args,
		     const SizeReq& size_req,
		     uint interval_s = UINT_MAX,
		     const CfgParser::Entry* section = nullptr);
	WidgetConfig(const WidgetConfig& cfg);
	~WidgetConfig(void);

	WidgetConfig& operator=(const WidgetConfig&);

	const std::string& getName(void) const { return _name; }
	const std::string& getArg(uint arg) const;
	const SizeReq& getSizeReq(void) const { return _size_req; }
	uint getIntervalS(void) const { return _interval_s; }

	const CfgParser::Entry* getCfgSection(void) const { return _section; }

private:
	/** Widget type name. */
	std::string _name;
	/** Widget arguments (if any). */
	std::vector<std::string> _args;
	/** Requested size of widget. */
	SizeReq _size_req;
	/** Refresh interval of widgets, set to UINT_MAX for non time
	    based widgets. */
	uint _interval_s;
	/** Configuration section, accessible for widget-specific
	    configuration. */
	CfgParser::Entry* _section;
};

/**
 * Configuration for commands to be run at given intervals to
 * collect data.
 */
class CommandConfig {
public:
	CommandConfig(const std::string& command,
		      uint interval_s, const std::string& assign);
	~CommandConfig();

	const std::string& getCommand() const { return _command; }
	uint getIntervalS() const { return _interval_s; }
	const std::string& getAssign() const { return _assign; }

private:
	/** Command to run (using the shell) */
	std::string _command;
	/** Interval between runs, not including run time. */
	uint _interval_s;
	/** If non-empty, assign all line output to variable. */
	std::string _assign;
};

/**
 * Configuration for panel, read from ~/.pekwm/panel by default.
 */
class PanelConfig {
public:
	typedef std::vector<CommandConfig> command_config_vector;
	typedef command_config_vector::const_iterator command_config_it;

	typedef std::vector<WidgetConfig> widget_config_vector;
	typedef widget_config_vector::const_iterator widget_config_it;

	PanelConfig(void);
	~PanelConfig(void);

	bool load(const std::string &panel_file);

	PanelPlacement getPlacement(void) const { return _placement; }
	int getHead(void) const { return _head; }

	uint getRefreshIntervalS(void) const { return _refresh_interval_s; }

	command_config_it commandsBegin(void) const {
		return _commands.begin();
	}
	command_config_it commandsEnd(void) const { return _commands.end(); }

	widget_config_it widgetsBegin(void) const { return _widgets.begin(); }
	widget_config_it widgetsEnd(void) const { return _widgets.end(); }

private:
	void loadPanel(CfgParser::Entry *section);
	void loadCommands(CfgParser::Entry *section);
	void loadWidgets(CfgParser::Entry *section);
	void addWidget(const std::string& name,
		       const SizeReq& size_req, uint interval,
		       const std::string& args_str,
		       const CfgParser::Entry* section);
	SizeReq parseSize(const std::string& size);
	uint calculateRefreshIntervalS(void) const;

private:
	/** Position of panel. */
	PanelPlacement _placement;
	/** Panel head, -1 for stretch all heads which is default. */
	int _head;

	/** List of commands to run. */
	command_config_vector _commands;
	/** List of widgets to instantiate. */
	std::vector<WidgetConfig> _widgets;
	/** At what given interval is refresh required at a minimum. */
	uint _refresh_interval_s;
};


#endif // _PEKWM_PANEL_PANEL_CONFIG_HH_
