//
// XSettings.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParser.hh"
#include "Charset.hh"
#include "Debug.hh"
#include "Util.hh"
#include "XSettings.hh"

std::string
_quote(const std::string &str_in)
{
	std::string str;
	Charset::Utf8Iterator it(str_in);
	for (; ! it.end(); ++it) {
		const char *c = *it;
		if (c[1] == '\0' && (c[0] == '\\' || c[0] == '"')) {
			str += '\\';
		}
		str += c;
	}
	return str;
}

void
XSetting::write(std::ostream &os, const std::string &name)
{
	os.put(_type);
	os.put(0);
	uint16_t name_len = static_cast<uint16_t>(name.size());
	os.write(reinterpret_cast<char*>(&name_len), 2);
	os.write(name.c_str(), name_len);
	writePad(os, name_len, 4);
	os.write(reinterpret_cast<char*>(&_last_changed), 4);
}

void
XSetting::writePad(std::ostream &os, size_t len, size_t multiple)
{
	int pad_bytes = len % multiple;
	if (pad_bytes != 0) {
		pad_bytes = 4 - pad_bytes;
	}
	for (int i = 0; i < pad_bytes; i++) {
		os.put(0);
	}
}

/**
 * Validate name against XSETTINGS specification.
 *
 * (A-Z, a-z, 0-9, _ and /, no leading or ending /, no double //, must
 *  start with A-Z or a-z, not empty)
 */
bool
XSetting::validateName(const std::string &name, std::string &error)
{
	error = "";
	if (name.empty()) {
		error = "name must not be empty";
	} else if (! ((name[0] >= 'A' && name[0] <= 'Z')
		      || (name[0] >= 'a' && name[0] <= 'z'))) {
		error = "name must start with a-z, A-Z";
	} else if (name[name.size() - 1] == '/') {
		error = "name must not end with /";
	}

	int last_slash = -1;;
	int name_size = static_cast<int>(name.size());
	for (int i = 1; error.empty() && i < name_size; i++) {
		if (name[i] == '/' && last_slash == (i - 1)) {
			error = "name must not contain //";
		} else if (name[i] == '/') {
			last_slash = i;
		} else if (! ((name[i] >= 'A' && name[i] <= 'Z')
			      || (name[i] >= 'a' && name[i] <= 'z')
			      || (name[i] >= '0' && name[i] <= '9'))) {
			error = "name can only contain a-z, A-Z, 0-9 and /";
		}
	}

	return error.empty();
}

void
XSettingString::write(std::ostream &os, const std::string &name)
{
	XSetting::write(os, name);
	uint32_t value_len = static_cast<uint32_t>(_value.size());
	os.write(reinterpret_cast<char*>(&value_len), 4);
	os.write(_value.c_str(), value_len);
	writePad(os, value_len, 4);
}

std::string
XSettingString::toString() const
{
	std::string str("s");
	return str + _quote(_value);
}

void
XSettingInteger::write(std::ostream &os, const std::string &name)
{
	XSetting::write(os, name);
	os.write(reinterpret_cast<char*>(&_value), 4);
}

std::string
XSettingInteger::toString() const
{
	std::string str("i");
	return str + std::to_string(_value);
}

void
XSettingColor::write(std::ostream &os, const std::string &name)
{
	XSetting::write(os, name);
	os.write(reinterpret_cast<char*>(&_r), 2);
	os.write(reinterpret_cast<char*>(&_b), 2);
	os.write(reinterpret_cast<char*>(&_g), 2);
	os.write(reinterpret_cast<char*>(&_a), 2);
}

std::string
XSettingColor::toString() const
{
	std::stringstream buf;
	buf << "c(" << _r << ", " << _g << ", " << _b << ", " << _a << ")";
	return buf.str();
}

XSettingColor*
mkXSettingColor(const std::string &str)
{
	if (str.size() < 10 || str[0] != 'c'
	    || str[1] != '(' || str[str.size() - 1] != ')') {
		return nullptr;
	}

	std::vector<std::string> toks;
	if (Util::splitString(str.substr(2, str.size() - 3), toks,
			      ",", 0, true) != 4) {
		return nullptr;
	}

	try {
		uint16_t r = std::stoi(toks[0]);
		uint16_t g = std::stoi(toks[1]);
		uint16_t b = std::stoi(toks[2]);
		uint16_t a = std::stoi(toks[3]);
		return new XSettingColor(r, g, b, a);
	} catch (std::invalid_argument&) {
		return nullptr;
	}
}

XSetting*
mkXSetting(const std::string &str)
{
	if (str.empty()) {
		return nullptr;
	}

	switch (str[0]) {
	case 's':
		return new XSettingString(str.substr(1));
	case 'i':
		try {
			int value = std::stoi(str.substr(1));
			return new XSettingInteger(value);
		} catch (std::invalid_argument&) {
			return nullptr;
		}
	case 'c':
		return mkXSettingColor(str);
	default:
		return nullptr;
	}
}

XSettings::XSettings()
	: _session_atom(None),
	  _owner(false)
{
	_window = X11::createWmWindow(X11::getRoot(), -200, -200, 5, 5,
				      InputOutput, PropertyChangeMask);
}

XSettings::~XSettings()
{
	map::iterator it(_settings.begin());
	for (; it != _settings.end(); ++it) {
		delete it->second;
	}
	X11::destroyWindow(_window);
}

bool
XSettings::setServerOwner()
{
	_owner = false;

	std::string screen_num = std::to_string(X11::getScreenNum());
	std::string session_name("_XSETTINGS_S" + screen_num);
	_session_atom = X11::getAtomId(session_name);
	X11::grabServer();
	Window session_owner = X11::getSelectionOwner(_session_atom);
	if (session_owner != None) {
		P_LOG(session_name << " already owned by "
		      << session_owner << ", not claiming ownership");
		X11::selectInput(session_owner, StructureNotifyMask);
		X11::ungrabServer(false);
		return false;
	}

	Time timestamp;
	getTime(timestamp);
	X11::setSelectionOwner(_session_atom, _window, timestamp);
	X11::ungrabServer(false);

	session_owner = X11::getSelectionOwner(_session_atom);
	if (session_owner != _window) {
		P_LOG("failed to set ownership on " << session_name);
		return false;
	}
	_owner = true;
	P_LOG("updated xsettings owner " << session_name << " to "
	      << _window);
	sendOwnerMessage(timestamp);

	return true;
}

void
XSettings::clearServerOwner()
{
	if (_owner) {
		Time timestamp;
		getTime(timestamp);
		X11::setSelectionOwner(_session_atom, None);
	}
	_owner = false;
}

void
XSettings::sendOwnerMessage(Time timestamp)
{
	X11::sendEvent(X11::getRoot(), X11::getRoot(), X11::getAtom(MANAGER),
		       StructureNotifyMask, timestamp, _session_atom, _window);
}

void
XSettings::updateServer()
{
	std::stringstream buf;
	Time timestamp;
	unsigned long serial = getTime(timestamp);
	writeProp(buf, serial);

	Atom atom = X11::getAtom(XSETTINGS_SETTINGS);
	const std::string &val = buf.str();
	const uchar *val_c = reinterpret_cast<const uchar*>(val.c_str());
	X11::changeProperty(_window, atom, atom, 8, PropModeReplace,
			    val_c, val.size());
	P_TRACE("set _XSETTINGS_SETTINGS on " << std::hex << _window
		<< std::dec << ", wrote " << val.size() << " bytes");
}

void
XSettings::selectOwnerDestroyInput()
{
	X11::grabServer();
	Window session_owner = X11::getSelectionOwner(_session_atom);
	if (session_owner != None) {
		X11::selectInput(session_owner, StructureNotifyMask);
	}
	X11::ungrabServer(false);
}

bool
XSettings::load(const std::string &source, CfgParserSource::Type type)
{
	P_TRACE("loading XSETTINGS from " << source);
	CfgParser cfg(CfgParserOpt(""));
	if (! cfg.parse(source, type)) {
		return false;
	}

	CfgParser::Entry *section =
		cfg.getEntryRoot()->findSection("SETTINGS");
	if (section == nullptr) {
		return true;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		XSetting *setting = nullptr;
		std::string error;
		if (XSetting::validateName((*it)->getName(), error)) {
			setting = mkXSetting((*it)->getValue());
			if (! setting) {
				error = "invalid setting value";
			}
		}

		if (setting) {
			set((*it)->getName(), setting);
		} else {
			P_WARN("invalid setting " << (*it)->getValue()
			       << " at " << (*it)->getSourceName()
			       << ":" << (*it)->getLine()
			       << ": " << error);
		}
	}
	return true;
}

bool
XSettings::save(const std::string &path)
{
	P_TRACE("saving XSETTINGS to " << path);
	std::ofstream os(path.c_str());
	if (! os.good()) {
		return false;
	}
	save(os);
	os.close();
	return true;
}

void
XSettings::save(std::ostream &os)
{
	os << "# written by pekwm_sys, overwritten by Sys XSave" << std::endl;
	os << "Settings {" << std::endl;
	map::iterator it(_settings.begin());
	for (; it != _settings.end(); ++it) {
		os << "\t\"" << _quote(it->first) << "\" = \""
		   << it->second->toString() << "\"" << std::endl;
	}
	os << "}" << std::endl;
}

void
XSettings::writeProp(std::ostream &os, unsigned long serial)
{
	int n = 1;
	if (reinterpret_cast<char*>(&n)[0]) {
		os.put(LSBFirst);
	} else {
		os.put(MSBFirst);
	}
	os.put(0);
	os.put(0);
	os.put(0);
	os.write(reinterpret_cast<char*>(&serial), 4);
	uint32_t num_settings = static_cast<uint32_t>(_settings.size());
	os.write(reinterpret_cast<char*>(&num_settings), 4);

	map::iterator it(_settings.begin());
	for (; it != _settings.end(); ++it) {
		it->second->write(os, it->first);
	}
}

/**
 * Get setting with the provided key, returns nullptr if it is not set.
 */
const XSetting*
XSettings::get(const std::string &name) const
{
	map::const_iterator it(_settings.find(name));
	return it == _settings.end() ? nullptr : it->second;
}

/**
 * Add string setting, overwrites existing value if any.
 */
void
XSettings::setString(const std::string &name, const std::string &value)
{
	set(name, new XSettingString(value));
}

/**
 * Add int32 setting, overwrites existing value if any.
 */
void
XSettings::setInt32(const std::string &name, int32_t value)
{
	set(name, new XSettingInteger(value));
}

/**
 * Add color setting, overwrites existing value if any.
 */
void
XSettings::setColor(const std::string &name, uint16_t r, uint16_t g,
		    uint16_t b, uint16_t a)
{
	set(name, new XSettingColor(r, g, b, a));
}

/**
 * Set property in settings, NOT updating value on X server.
 */
void
XSettings::set(const std::string &name, XSetting *setting)
{
	map::iterator it(_settings.find(name));
	if (it == _settings.end()) {
		_settings[name] = setting;
	} else {
		if (setting->getLastChanged() == 0) {
			// last changed not set and previous value exist,
			// increment last changed.
			uint32_t last_changed =
				it->second->getLastChanged() + 1;
			setting->setLastChanged(last_changed);
		}
		delete it->second;
		it->second = setting;
	}
}

/**
 * Remove setting, independent of type, from configured settings.
 */
void
XSettings::remove(const std::string &name)
{
	map::iterator it(_settings.find(name));
	if (it != _settings.end()) {
		delete it->second;
		_settings.erase(it);
	}
}

/**
 * Get current time of server by generating an event and reading the
 * timestamp on it.
 *
 * @return Time on server.
 */
unsigned long
XSettings::getTime(Time &cur_time) const
{
	X11::changeProperty(_window, X11::getAtom(WM_CLASS),
			    X11::getAtom(STRING), 8, PropModeAppend, 0, 0);
	XEvent event;
	X11::getWindowEvent(_window, PropertyChangeMask, event);
	cur_time = event.xproperty.time;

	return event.xproperty.serial;
}
