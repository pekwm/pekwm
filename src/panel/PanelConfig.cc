//
// PanelConfig.cc for pekwm
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

//#include "Config.hh"
#include "Debug.hh"
#include "PanelConfig.hh"

/** empty string, used as default return value. */
static std::string _empty_string;

static Util::StringTo<PanelPlacement> panel_placement_map[] =
	{{"TOP", PANEL_TOP},
	 {"BOTTOM", PANEL_BOTTOM},
	 {nullptr, PANEL_TOP}};


// WidgetConfig

WidgetConfig::WidgetConfig(const std::string& name,
			   const std::vector<std::string>& args,
			   const SizeReq& size_req,
			   uint interval_s,
			   const CfgParser::Entry* section)
	: _name(name),
	  _args(args),
	  _size_req(size_req),
	  _interval_s(interval_s),
	  _section(nullptr)
{
	if (section) {
		_section = new CfgParser::Entry(*section);
	}
}

WidgetConfig::WidgetConfig(const WidgetConfig& cfg)
	: _name(cfg._name),
	  _args(cfg._args),
	  _size_req(cfg._size_req),
	  _interval_s(cfg._interval_s)
{
	if (cfg._section) {
		_section = new CfgParser::Entry(*cfg._section);
	} else {
		_section = nullptr;
	}
}

WidgetConfig::~WidgetConfig(void)
{
	delete _section;
}

WidgetConfig&
WidgetConfig::operator=(const WidgetConfig& rhs)
{
	if (this == &rhs) {
		return *this;
	}

	delete _section;

	_name = rhs._name;
	_args = rhs._args;
	_size_req = rhs._size_req;
	_interval_s = rhs._interval_s;

	if (rhs._section) {
		_section = new CfgParser::Entry(*rhs._section);
	} else {
		_section = nullptr;
	}

	return *this;
}

const std::string&
WidgetConfig::getArg(uint arg) const
{
	return arg < _args.size() ? _args[arg] : _empty_string;
}


// CommandConfig

CommandConfig::CommandConfig(const std::string& command,
			     uint interval_s)
	: _command(command),
	  _interval_s(interval_s)
{
}

CommandConfig::~CommandConfig(void)
{
}

// PanelConfig

PanelConfig::PanelConfig(void)
	: _placement(DEFAULT_PLACEMENT),
	  _head(-1)
{
}

PanelConfig::~PanelConfig(void)
{
}

bool
PanelConfig::load(const std::string &panel_file)
{
	CfgParser cfg(CfgParserOpt("")); // FIXME: pekwm::configScriptPath());
	if (! cfg.parse(panel_file, CfgParserSource::SOURCE_FILE, true)) {
		return false;
	}

	CfgParser::Entry *root = cfg.getEntryRoot();
	loadPanel(root->findSection("PANEL"));
	loadCommands(root->findSection("COMMANDS"));
	loadWidgets(root->findSection("WIDGETS"));
	_refresh_interval_s = calculateRefreshIntervalS();
	return true;
}

void
PanelConfig::loadPanel(CfgParser::Entry *section)
{
	if (section == nullptr) {
		return;
	}

	std::string placement;
	CfgParserKeys keys;
	keys.add_string("PLACEMENT", placement, "TOP");
	keys.add_numeric<int>("HEAD", _head, -1);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	_placement = Util::StringToGet(panel_placement_map, placement);
}

void
PanelConfig::loadCommands(CfgParser::Entry *section)
{
	_commands.clear();
	if (section == nullptr) {
		return;
	}

	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		uint interval = UINT_MAX;

		if ((*it)->getSection()) {
			CfgParserKeys keys;
			keys.add_numeric<uint>("INTERVAL", interval, UINT_MAX);
			(*it)->getSection()->parseKeyValues(keys.begin(),
							    keys.end());
			keys.clear();
		}
		_commands.push_back(CommandConfig((*it)->getValue(), interval));
	}
}

void
PanelConfig::loadWidgets(CfgParser::Entry *section)
{
	_widgets.clear();
	if (section == nullptr) {
		return;
	}

	CfgParser::Entry::entry_cit it = section->begin();
	for (; it != section->end(); ++it) {
		uint interval = UINT_MAX;
		std::string size = "REQUIRED";

		CfgParser::Entry *w_section = (*it)->getSection();
		if (w_section) {
			CfgParserKeys keys;
			keys.add_numeric<uint>("INTERVAL", interval, UINT_MAX);
			keys.add_string("SIZE", size, "REQUIRED");
			w_section->parseKeyValues(keys.begin(), keys.end());
			keys.clear();
		}

		SizeReq size_req = parseSize(size);
		addWidget((*it)->getName(), size_req, interval,
			  (*it)->getValue(), w_section);
	}
}

void
PanelConfig::addWidget(const std::string& name,
		       const SizeReq& size_req, uint interval,
		       const std::string& args_str,
		       const CfgParser::Entry* section)
{
	std::vector<std::string> args;
	if (! args_str.empty()) {
		args.push_back(args_str);
	}
	_widgets.push_back(WidgetConfig(name, args, size_req, interval,
					section));
}

SizeReq
PanelConfig::parseSize(const std::string& size)
{
	std::vector<std::string> toks;
	Util::splitString(size, toks, " \t", 2);
	if (toks.size() == 1) {
		if (pekwm::ascii_ncase_equal("REQUIRED", toks[0])) {
			return SizeReq(WIDGET_UNIT_REQUIRED, 0);
		} else if (toks[0] == "*") {
			return SizeReq(WIDGET_UNIT_REST, 0);
		}
	} else if (toks.size() == 2) {
		if (pekwm::ascii_ncase_equal("PIXELS", toks[0])) {
			return SizeReq(WIDGET_UNIT_PIXELS,
				       std::stoi(toks[1]));
		} else if (pekwm::ascii_ncase_equal("PERCENT", toks[0])) {
			return SizeReq(WIDGET_UNIT_PERCENT,
				       std::stoi(toks[1]));
		} else if (pekwm::ascii_ncase_equal("TEXTWIDTH", toks[0])) {
			return SizeReq(toks[1]);
		}
	}

	USER_WARN("failed to parse size: " << size);
	return SizeReq(WIDGET_UNIT_REQUIRED, 0);
}

uint
PanelConfig::calculateRefreshIntervalS(void) const
{
	uint min = UINT_MAX;
	command_config_vector::const_iterator it = _commands.begin();
	for (; it != _commands.end(); ++it) {
		if (it->getIntervalS() < min) {
			min = it->getIntervalS();
		}
	}
	std::vector<WidgetConfig>::const_iterator w_it = _widgets.begin();
	for (; w_it != _widgets.end(); ++w_it) {
		if (w_it->getIntervalS() < min) {
			min = w_it->getIntervalS();
		}
	}
	return min;
}
