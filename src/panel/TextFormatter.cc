//
// TextFormatter.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "TextFormatter.hh"

#include <set>

/** empty string, used as default return value. */
static std::string _empty_string;

TextFormatter::TextFormatter(VarData& var_data, WmState& wm_state)
	: _var_data(var_data),
	  _wm_state(wm_state),
	  _check_wm_state(false)
{
}

TextFormatter::~TextFormatter(void)
{
}

/**
 * pre process format string for use with the format command, expands
 * environment variables and other static data.
 */
std::string
TextFormatter::preprocess(const std::string& raw_format)
{
	return format(raw_format, TextFormatter::tfPreprocessVar);
}

/**
 * format previously pre-processed format string expanding external
 * command data and wm state.
 */
std::string
TextFormatter::format(const std::string& pp_format)
{
	return format(pp_format, TextFormatter::tfExpandVar);
}

std::string
TextFormatter::format(const std::string& pp_format, formatFun exp)
{
	static std::set<char> var_end_chars;
	if (var_end_chars.empty()) {
		var_end_chars.insert('\'');
		var_end_chars.insert('"');
	}
	std::string formatted;

	bool in_escape = false, in_var = false;
	Charset::Utf8Iterator it(pp_format);
	std::string buf;
	for (; it.ok(); ++it) {
		if (in_escape) {
			buf += *it;
			in_escape = false;
		} else if (it == '\\') {
			in_escape = true;
		} else if (in_var
			   && it.charLen() == 1
			   && (isspace((*it)[0])
			       || var_end_chars.count((*it)[0]) != 0)) {
			formatted += exp(this, buf);
			buf = *it;
			in_var = false;
		} else if (! in_var && it == '%') {
			if (! buf.empty()) {
				formatted += buf;
				buf = _empty_string;
			}
			in_var = true;
		} else {
			buf += *it;
		}
	}
	if (! buf.empty()) {
		if (in_var) {
			formatted += exp(this, buf);
		} else {
			formatted += buf;
		}
	}

	return formatted;
}

std::string
TextFormatter::preprocessVar(const std::string& buf)
{
	if (buf.empty()) {
		return "%";
	}

	switch (buf[0]) {
	case '_': {
		return Util::getEnv(buf.substr(1));
	}
	case ':':
		_check_wm_state = true;
		return "%" + buf;
	default:
		_fields.push_back(buf);
		return "%" + buf;
	}
}

std::string
TextFormatter::expandVar(const std::string& buf)
{
	if (buf.empty()) {
		return _empty_string;
	}

	if (buf[0] == ':') {
		// window manager state variable
		if (buf == ":CLIENT_NAME:") {
			Window win = _wm_state.getActiveWindow();
			ClientInfo *client_info =
				_wm_state.findClientInfo(win);
			return client_info
				? client_info->getName()
				: _empty_string;
		} else if (buf == ":WORKSPACE_NUMBER:") {
			return std::to_string(
					_wm_state.getActiveWorkspace() + 1);
		} else if (buf == ":WORKSPACE_NAME:") {
			return _wm_state.getWorkspaceName(
					_wm_state.getActiveWorkspace());
		}
		return _empty_string;
	}

	// external command data
	return _var_data.get(buf);
}

TextFormatObserver::TextFormatObserver(VarData& var_data, WmState& wm_state,
				       Observer *observer,
				       const std::string& format)
	: _tf(var_data, wm_state),
	  _observer(observer),
	  _pp_format(_tf.preprocess(format))
{
	if (! _tf.getFields().empty()) {
		pekwm::observerMapping()->addObserver(
			_tf.getVarData(), _observer, 100);
	}
	if (_tf.referenceWmState()) {
		pekwm::observerMapping()->addObserver(
			_tf.getWmState(), _observer, 100);
	}
}

TextFormatObserver::~TextFormatObserver()
{
	if (_tf.referenceWmState()) {
		pekwm::observerMapping()->removeObserver(
			_tf.getWmState(), _observer);
	}
	if (! _tf.getFields().empty()) {
		pekwm::observerMapping()->removeObserver(
			_tf.getVarData(), _observer);
	}
}

/**
 * Check if the observation matches any of the observed fields.
 */
bool
TextFormatObserver::match(Observation *observation)
{
	if (isFixed()) {
		return false;
	}

	FieldObservation *fo = dynamic_cast<FieldObservation*>(observation);
	if (fo != nullptr) {
		const std::vector<std::string> &fields = _tf.getFields();
		std::vector<std::string>::const_iterator it(fields.begin());
		for (; it != fields.end(); ++it) {
			if (*it == fo->getField()) {
				return true;
			}
		}
	}
	return false;
}
